static inline void clear_user_page(void *addr, unsigned long vaddr,
        struct page *page)
{
        clear_page(addr);
        if (pages_do_alias((unsigned long) addr, vaddr & PAGE_MASK))
                flush_dcache_page(page);
}

static inline void copy_user_page(void *to, void *from, unsigned long vaddr,
	struct page *page)
{
	copy_page(to, from);
	if (pages_do_alias((unsigned long) to, vaddr & PAGE_MASK))
		flush_dcache_page(page);
}
