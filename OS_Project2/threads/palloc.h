#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002,             /* Zero page contents. */
    PAL_USER = 004              /* User page. */
  };

void palloc_init (size_t user_page_limit); //페이지 할당자를 초기화, 커널페이지와 사용자페이지로 나눠서 각자 할당한다.
void *palloc_get_page (enum palloc_flags);// 단일 페이지 할당
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);// page_cnt 인자로 주어진 수 만큼 페이지 할당, 할당은 연속된 메모리 공간을 스캔한 뒤에 연속된 페이지를
//할당한다.
void palloc_free_page (void *); //단일페이지 free
void palloc_free_multiple (void *, size_t page_cnt); // 다수페이지를 free
void palloc_get_status (enum palloc_flags flags);//할당된 페이지들의 상태를 출력할 함수. 우리가 구현해야함.

#endif /* threads/palloc.h */
