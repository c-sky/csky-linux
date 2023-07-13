// SPDX-License-Identifier: GPL-2.0-only
/*
 * KSM functional tests
 *
 * Copyright 2022, Red Hat, Inc.
 *
 * Author(s): David Hildenbrand <david@redhat.com>
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/userfaultfd.h>

#include "../kselftest.h"
#include "vm_util.h"

#define KiB 1024u
#define MiB (1024 * KiB)

static int ksm_fd;
static int ksm_full_scans_fd;
static int pagemap_fd;
static size_t pagesize;

static bool range_maps_duplicates(char *addr, unsigned long size)
{
	unsigned long offs_a, offs_b, pfn_a, pfn_b;

	/*
	 * There is no easy way to check if there are KSM pages mapped into
	 * this range. We only check that the range does not map the same PFN
	 * twice by comparing each pair of mapped pages.
	 */
	for (offs_a = 0; offs_a < size; offs_a += pagesize) {
		pfn_a = pagemap_get_pfn(pagemap_fd, addr + offs_a);
		/* Page not present or PFN not exposed by the kernel. */
		if (pfn_a == -1ul || !pfn_a)
			continue;

		for (offs_b = offs_a + pagesize; offs_b < size;
		     offs_b += pagesize) {
			pfn_b = pagemap_get_pfn(pagemap_fd, addr + offs_b);
			if (pfn_b == -1ul || !pfn_b)
				continue;
			if (pfn_a == pfn_b)
				return true;
		}
	}
	return false;
}

static long ksm_get_full_scans(void)
{
	char buf[10];
	ssize_t ret;

	ret = pread(ksm_full_scans_fd, buf, sizeof(buf) - 1, 0);
	if (ret <= 0)
		return -errno;
	buf[ret] = 0;

	return strtol(buf, NULL, 10);
}

static int ksm_merge(void)
{
	long start_scans, end_scans;

	/* Wait for two full scans such that any possible merging happened. */
	start_scans = ksm_get_full_scans();
	if (start_scans < 0)
		return start_scans;
	if (write(ksm_fd, "1", 1) != 1)
		return -errno;
	do {
		end_scans = ksm_get_full_scans();
		if (end_scans < 0)
			return end_scans;
	} while (end_scans < start_scans + 2);

	return 0;
}

static char *mmap_and_merge_range(char val, unsigned long size, bool use_prctl)
{
	char *map;
	int ret;

	map = mmap(NULL, size, PROT_READ|PROT_WRITE,
		   MAP_PRIVATE|MAP_ANON, -1, 0);
	if (map == MAP_FAILED) {
		ksft_test_result_fail("mmap() failed\n");
		return MAP_FAILED;
	}

	/* Don't use THP. Ignore if THP are not around on a kernel. */
	if (madvise(map, size, MADV_NOHUGEPAGE) && errno != EINVAL) {
		ksft_test_result_fail("MADV_NOHUGEPAGE failed\n");
		goto unmap;
	}

	/* Make sure each page contains the same values to merge them. */
	memset(map, val, size);

	if (use_prctl) {
		ret = prctl(PR_SET_MEMORY_MERGE, 1, 0, 0, 0);
		if (ret < 0 && errno == EINVAL) {
			ksft_test_result_skip("PR_SET_MEMORY_MERGE not supported\n");
			goto unmap;
		} else if (ret) {
			ksft_test_result_fail("PR_SET_MEMORY_MERGE=1 failed\n");
			goto unmap;
		}
	} else if (madvise(map, size, MADV_MERGEABLE)) {
		ksft_test_result_fail("MADV_MERGEABLE failed\n");
		goto unmap;
	}

	/* Run KSM to trigger merging and wait. */
	if (ksm_merge()) {
		ksft_test_result_fail("Running KSM failed\n");
		goto unmap;
	}
	return map;
unmap:
	munmap(map, size);
	return MAP_FAILED;
}

static void test_unmerge(void)
{
	const unsigned int size = 2 * MiB;
	char *map;

	ksft_print_msg("[RUN] %s\n", __func__);

	map = mmap_and_merge_range(0xcf, size, false);
	if (map == MAP_FAILED)
		return;

	if (madvise(map, size, MADV_UNMERGEABLE)) {
		ksft_test_result_fail("MADV_UNMERGEABLE failed\n");
		goto unmap;
	}

	ksft_test_result(!range_maps_duplicates(map, size),
			 "Pages were unmerged\n");
unmap:
	munmap(map, size);
}

static void test_unmerge_discarded(void)
{
	const unsigned int size = 2 * MiB;
	char *map;

	ksft_print_msg("[RUN] %s\n", __func__);

	map = mmap_and_merge_range(0xcf, size, false);
	if (map == MAP_FAILED)
		return;

	/* Discard half of all mapped pages so we have pte_none() entries. */
	if (madvise(map, size / 2, MADV_DONTNEED)) {
		ksft_test_result_fail("MADV_DONTNEED failed\n");
		goto unmap;
	}

	if (madvise(map, size, MADV_UNMERGEABLE)) {
		ksft_test_result_fail("MADV_UNMERGEABLE failed\n");
		goto unmap;
	}

	ksft_test_result(!range_maps_duplicates(map, size),
			 "Pages were unmerged\n");
unmap:
	munmap(map, size);
}

#ifdef __NR_userfaultfd
static void test_unmerge_uffd_wp(void)
{
	struct uffdio_writeprotect uffd_writeprotect;
	const unsigned int size = 2 * MiB;
	struct uffdio_api uffdio_api;
	char *map;
	int uffd;

	ksft_print_msg("[RUN] %s\n", __func__);

	map = mmap_and_merge_range(0xcf, size, false);
	if (map == MAP_FAILED)
		return;

	/* See if UFFD is around. */
	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd < 0) {
		ksft_test_result_skip("__NR_userfaultfd failed\n");
		goto unmap;
	}

	/* See if UFFD-WP is around. */
	uffdio_api.api = UFFD_API;
	uffdio_api.features = UFFD_FEATURE_PAGEFAULT_FLAG_WP;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) < 0) {
		ksft_test_result_fail("UFFDIO_API failed\n");
		goto close_uffd;
	}
	if (!(uffdio_api.features & UFFD_FEATURE_PAGEFAULT_FLAG_WP)) {
		ksft_test_result_skip("UFFD_FEATURE_PAGEFAULT_FLAG_WP not available\n");
		goto close_uffd;
	}

	/* Register UFFD-WP, no need for an actual handler. */
	if (uffd_register(uffd, map, size, false, true, false)) {
		ksft_test_result_fail("UFFDIO_REGISTER_MODE_WP failed\n");
		goto close_uffd;
	}

	/* Write-protect the range using UFFD-WP. */
	uffd_writeprotect.range.start = (unsigned long) map;
	uffd_writeprotect.range.len = size;
	uffd_writeprotect.mode = UFFDIO_WRITEPROTECT_MODE_WP;
	if (ioctl(uffd, UFFDIO_WRITEPROTECT, &uffd_writeprotect)) {
		ksft_test_result_fail("UFFDIO_WRITEPROTECT failed\n");
		goto close_uffd;
	}

	if (madvise(map, size, MADV_UNMERGEABLE)) {
		ksft_test_result_fail("MADV_UNMERGEABLE failed\n");
		goto close_uffd;
	}

	ksft_test_result(!range_maps_duplicates(map, size),
			 "Pages were unmerged\n");
close_uffd:
	close(uffd);
unmap:
	munmap(map, size);
}
#endif

/* Verify that KSM can be enabled / queried with prctl. */
static void test_prctl(void)
{
	int ret;

	ksft_print_msg("[RUN] %s\n", __func__);

	ret = prctl(PR_SET_MEMORY_MERGE, 1, 0, 0, 0);
	if (ret < 0 && errno == EINVAL) {
		ksft_test_result_skip("PR_SET_MEMORY_MERGE not supported\n");
		return;
	} else if (ret) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=1 failed\n");
		return;
	}

	ret = prctl(PR_GET_MEMORY_MERGE, 0, 0, 0, 0);
	if (ret < 0) {
		ksft_test_result_fail("PR_GET_MEMORY_MERGE failed\n");
		return;
	} else if (ret != 1) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=1 not effective\n");
		return;
	}

	ret = prctl(PR_SET_MEMORY_MERGE, 0, 0, 0, 0);
	if (ret) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=0 failed\n");
		return;
	}

	ret = prctl(PR_GET_MEMORY_MERGE, 0, 0, 0, 0);
	if (ret < 0) {
		ksft_test_result_fail("PR_GET_MEMORY_MERGE failed\n");
		return;
	} else if (ret != 0) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=0 not effective\n");
		return;
	}

	ksft_test_result_pass("Setting/clearing PR_SET_MEMORY_MERGE works\n");
}

/* Verify that prctl ksm flag is inherited. */
static void test_prctl_fork(void)
{
	int ret, status;
	pid_t child_pid;

	ksft_print_msg("[RUN] %s\n", __func__);

	ret = prctl(PR_SET_MEMORY_MERGE, 1, 0, 0, 0);
	if (ret < 0 && errno == EINVAL) {
		ksft_test_result_skip("PR_SET_MEMORY_MERGE not supported\n");
		return;
	} else if (ret) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=1 failed\n");
		return;
	}

	child_pid = fork();
	if (!child_pid) {
		exit(prctl(PR_GET_MEMORY_MERGE, 0, 0, 0, 0));
	} else if (child_pid < 0) {
		ksft_test_result_fail("fork() failed\n");
		return;
	}

	if (waitpid(child_pid, &status, 0) < 0) {
		ksft_test_result_fail("waitpid() failed\n");
		return;
	} else if (WEXITSTATUS(status) != 1) {
		ksft_test_result_fail("unexpected PR_GET_MEMORY_MERGE result in child\n");
		return;
	}

	if (prctl(PR_SET_MEMORY_MERGE, 0, 0, 0, 0)) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=0 failed\n");
		return;
	}

	ksft_test_result_pass("PR_SET_MEMORY_MERGE value is inherited\n");
}

static void test_prctl_unmerge(void)
{
	const unsigned int size = 2 * MiB;
	char *map;

	ksft_print_msg("[RUN] %s\n", __func__);

	map = mmap_and_merge_range(0xcf, size, true);
	if (map == MAP_FAILED)
		return;

	if (prctl(PR_SET_MEMORY_MERGE, 0, 0, 0, 0)) {
		ksft_test_result_fail("PR_SET_MEMORY_MERGE=0 failed\n");
		goto unmap;
	}

	ksft_test_result(!range_maps_duplicates(map, size),
			 "Pages were unmerged\n");
unmap:
	munmap(map, size);
}

int main(int argc, char **argv)
{
	unsigned int tests = 5;
	int err;

#ifdef __NR_userfaultfd
	tests++;
#endif

	ksft_print_header();
	ksft_set_plan(tests);

	pagesize = getpagesize();

	ksm_fd = open("/sys/kernel/mm/ksm/run", O_RDWR);
	if (ksm_fd < 0)
		ksft_exit_skip("open(\"/sys/kernel/mm/ksm/run\") failed\n");
	ksm_full_scans_fd = open("/sys/kernel/mm/ksm/full_scans", O_RDONLY);
	if (ksm_full_scans_fd < 0)
		ksft_exit_skip("open(\"/sys/kernel/mm/ksm/full_scans\") failed\n");
	pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
	if (pagemap_fd < 0)
		ksft_exit_skip("open(\"/proc/self/pagemap\") failed\n");

	test_unmerge();
	test_unmerge_discarded();
#ifdef __NR_userfaultfd
	test_unmerge_uffd_wp();
#endif

	test_prctl();
	test_prctl_fork();
	test_prctl_unmerge();

	err = ksft_get_fail_cnt();
	if (err)
		ksft_exit_fail_msg("%d out of %d tests failed\n",
				   err, ksft_test_num());
	return ksft_exit_pass();
}
