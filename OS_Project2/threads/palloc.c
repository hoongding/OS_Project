#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <list.h>
//! Kernel 만 신경쓰면된다!!!! User는 가상메모리영역임...
/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  See malloc.h for an allocator that
   hands out smaller chunks.

   System memory is divided into two "pools" called the kernel
   and user pools.  The user pool is for user (virtual) memory
   pages, the kernel pool for everything else.  The idea here is
   that the kernel needs to have memory for its own operations
   even if user processes are swapping like mad.

   By default, half of system RAM is given to the kernel pool and
   half to the user pool.  That should be huge overkill for the
   kernel pool, but that's just fine for demonstration purposes. */

/* A memory pool. */
struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    struct bitmap *used_map;            /* Bitmap of free pages. */
    uint8_t *base;                      /* Base of pool. */
  };



/* Two pools: one for kernel data, one for user pages. */
static struct pool kernel_pool, user_pool;

static void init_pool (struct pool *, void *base, size_t page_cnt,const char *name);
static bool page_from_pool (const struct pool *, void *page);

/* Initializes the page allocator.  At most USER_PAGE_LIMIT
   pages are put into the user pool. */
void
palloc_init (size_t user_page_limit)
{
  /* Free memory starts at 1 MB and runs to the end of RAM. */
  uint8_t *free_start = ptov (1024 * 1024);
  uint8_t *free_end = ptov (init_ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  size_t user_pages = free_pages / 2;
  size_t kernel_pages;
  if (user_pages > user_page_limit)
    user_pages = user_page_limit;
  kernel_pages = free_pages - user_pages;

  /* Give half of memory to kernel, half to user. */
  init_pool (&kernel_pool, free_start, kernel_pages, "kernel pool");
  init_pool (&user_pool, free_start + kernel_pages * PGSIZE,
             user_pages, "user pool");
}

size_t get_power_of_two(size_t cnt){
  size_t size_val = 1;
  while(cnt > size_val){
    size_val = size_val * 2;
  }
  return size_val;
}

size_t buddy_allocation(struct bitmap *b, size_t start, size_t cnt, bool value){
    size_t size_val = 1;
    size_t idx = 0;//inx는 0부터 시작해서 할당가능한지 여부를 판단.
    while(cnt > size_val){ //cnt가 2의 진수보다 클때까지 2를 곱해버려
      size_val = size_val * 2;
    }
    while(idx <= bitmap_size(b)){
      if(!bitmap_contains(b, idx, size_val, !value)){
         bitmap_set_multiple (b, idx, size_val, !value);//cnt 2의 진수로 안받아도 2의 진수만큼 페이지 할당...
        return idx;
      }
      else{
        idx += size_val;
      }
    }
    return BITMAP_ERROR;
}
/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   If PAL_USER is set, the pages are obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the pages are filled with zeros.  If too few pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  void *pages;
  size_t page_idx;

  if (page_cnt == 0)
    return NULL;

  lock_acquire (&pool->lock);
  page_idx = buddy_allocation(pool->used_map, 0, page_cnt, false);
  lock_release (&pool->lock);

  if (page_idx != BITMAP_ERROR)
    pages = pool->base + PGSIZE * page_idx;
    // pages = pool->base + 버디블록시작점더해주기;
  else
    pages = NULL;

  if (pages != NULL) 
    {
      if (flags & PAL_ZERO)
        memset (pages, 0, PGSIZE * page_cnt);
    }
  else 
    {
      if (flags & PAL_ASSERT)
        PANIC ("palloc_get: out of pages");
    }

  return pages;
}

/* Obtains a single free page and returns its kernel virtual
   address.
   If PAL_USER is set, the page is obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the page is filled with zeros.  If no pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_page (enum palloc_flags flags) 
{
  return palloc_get_multiple (flags, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
void
palloc_free_multiple (void *pages, size_t page_cnt) 
{
  //TODO : free해줄때 buddysystem에서 합치는 함수를 짜야함.
  
  struct pool *pool;
  size_t page_idx;
  size_t power_of_two = get_power_of_two(page_cnt);

  ASSERT (pg_ofs (pages) == 0);
  if (pages == NULL || power_of_two == 0)
    return;

  if (page_from_pool (&kernel_pool, pages))
    pool = &kernel_pool;
  else if (page_from_pool (&user_pool, pages))
    pool = &user_pool;
  else
    NOT_REACHED ();

  page_idx = pg_no (pages) - pg_no (pool->base);

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * power_of_two);
#endif

  ASSERT (bitmap_all (pool->used_map, page_idx, power_of_two));
  bitmap_set_multiple (pool->used_map, page_idx, power_of_two, false);
}

/* Frees the page at PAGE. */
void
palloc_free_page (void *page) 
{
  palloc_free_multiple (page, 1);
}

/* Initializes pool P as starting at START and ending at END,
   naming it NAME for debugging purposes. */
static void
init_pool (struct pool *p, void *base, size_t page_cnt, const char *name) 
{
  /* We'll put the pool's used_map at its base.
     Calculate the space needed for the bitmap
     and subtract it from the pool's size. */
  size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (page_cnt), PGSIZE);
  if (bm_pages > page_cnt)
    PANIC ("Not enough memory in %s for bitmap.", name);
  page_cnt -= bm_pages;

  printf ("%zu pages available in %s.\n", page_cnt, name);

  /* Initialize the pool. */
  lock_init (&p->lock);
  p->used_map = bitmap_create_in_buf (page_cnt, base, bm_pages * PGSIZE);
  p->base = base + bm_pages * PGSIZE;
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page) 
{
  size_t page_no = pg_no (page);
  size_t start_page = pg_no (pool->base);
  size_t end_page = start_page + bitmap_size (pool->used_map);

  return page_no >= start_page && page_no < end_page;
}



/* Obtains a status of the page pool */
void
palloc_get_status (enum palloc_flags flags)
{
  // TODO: IMPLEMENT THIS
  struct bitmap *kernel_bitmap = kernel_pool.used_map;
  size_t kernel_size = bitmap_size(kernel_bitmap);
  printf("Page Count : %d\n", kernel_size);
  int loop_num = kernel_size / 32;
  
  for (int i = 1; i <= 32; i++) //! 1~32까지 출력.(01 02 03 04)
  {
    if (i < 10)
    {
      printf("0%d ", i);
    }
    else
    {
      printf("%d ", i);
    }
  }
  printf("\n");
  int i;
  for (i = 0; i<kernel_size/32 ; i++)
  {
    for (int j = 0; j < 32; j++)
    {
      printf("%zu  ", bitmap_test(kernel_bitmap, 32 * i + j));
    }
    printf("\n");
    
  }
  for(int j = 0; j<kernel_size%32;j++){
    printf("%zu  ", bitmap_test(kernel_bitmap,32*i+j));
  }
  printf("\n\n");
  

}
