#define clear_user_page(addr, vaddr, page)      \
        do {                                    \
          clear_page(addr);                     \
        } while (0)

#define copy_user_page(to, from, vaddr, page)   \
        do {                                    \
          copy_page(to, from);                  \
        } while (0)

