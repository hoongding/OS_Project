#include <stdio.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"

//! create

int vehicles_num;
int success_to_lock;

const struct position vehicle_path[4][4][10] = {

    /* from A */ {//*서 (4,0)에서만 출발
                  // A에서 A로 가는건 X
                  {
                      {-1, -1},
                  },
                  // A에서 B로 가는 경로(우회전)
                  //!(4,2)
                  {
                      {4, 0},
                      {4, 1},
                      {4, 2},
                      {5, 2},
                      {6, 2},
                      {-1, -1},
                  },
                  // A에서 C로 가는 경로(직진))
                  //!(4,2),(4,3),(4,4)
                  {
                      {4, 0},
                      {4, 1},
                      {4, 2},
                      {4, 3},
                      {4, 4},
                      {4, 5},
                      {4, 6},
                      {-1, -1},
                  },
                  // A에서 D로 가는 경로(크게 좌회전)
                  //!(4,2),(4,3),(4,4),(3,4),(2,4)
                  {{4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {1, 4}, {0, 4}, {-1, -1}}},
    /* from B */ {//*남 (6.4)에서만 출발
                  // B에서 A로 가는 경로.(크게 좌회전)
                  //!(4,4),(3,4),(2,4),(2,3),(2,2) */
                  {{6, 4}, {5, 4}, {4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {2, 1}, {2, 0}, {-1, -1}},
                  // B에서 B는 갈 수 없음. */
                  {
                      {-1, -1},
                  },
                  // B에서 C로 우회전(작은 우회전)
                  //!(4,4) */
                  {
                      {6, 4},
                      {5, 4},
                      {4, 4},
                      {4, 5},
                      {4, 6},
                      {-1, -1},
                  },
                  // B에서 D로 직진
                  //!(4,4),(3,4),(2,4) */
                  {
                      {6, 4},
                      {5, 4},
                      {4, 4},
                      {3, 4},
                      {2, 4},
                      {1, 4},
                      {0, 4},
                      {-1, -1},
                  }},
    /* from C */ {//*동 (2,6)에서만 출발
                  // C에서 A로 직진
                  //!(2,4),(2,3),(2,2) */
                  {
                      {2, 6},
                      {2, 5},
                      {2, 4},
                      {2, 3},
                      {2, 2},
                      {2, 1},
                      {2, 0},
                      {-1, -1},
                  },
                  // C에서 B로 좌회전(큰)
                  //!(2,4),(2,3),(2,2),(3,2),(4,2) */
                  {{2, 6}, {2, 5}, {2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}, {-1, -1}},
                  // C에서 C로는 갈 수 없음 */
                  {
                      {-1, -1},
                  },
                  // C에서 D로 우회전(작은)
                  //!(2,4) */
                  {
                      {2, 6},
                      {2, 5},
                      {2, 4},
                      {1, 4},
                      {0, 4},
                      {-1, -1},
                  }},
    /* from D */ {//*북 (0,2)에서만 출발
                  // D에서 A로 우회전 (작은)
                  //!(2,2) */
                  {
                      {0, 2},
                      {1, 2},
                      {2, 2},
                      {2, 1},
                      {2, 0},
                      {-1, -1},
                  },
                  // D에서 B로 직진
                  //!(2,2),(3,2),(4,2) */
                  {
                      {0, 2},
                      {1, 2},
                      {2, 2},
                      {3, 2},
                      {4, 2},
                      {5, 2},
                      {6, 2},
                      {-1, -1},
                  },
                  // D에서 C로 좌회전(큰)
                  //!(2,2),(3,2),(4,2),(4,3),(4,4) */
                  {{0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}, {-1, -1}},
                  // D에서 D로는 갈 수 없음. */
                  {
                      {-1, -1},
                  }}};

const struct position intersection_pos[8] = {
    {2, 2}, {2, 3}, {2, 4}, {3, 2}, {3, 4}, {4, 2}, {4, 3}, {4, 4}};

static int at_crossroad(struct position pos)
{
    int return_val = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        if ((pos.row == intersection_pos[i].row) && (pos.col == intersection_pos[i].col)) //만약 지금 위치가 교차로에 있다면
        {
            return_val = 1;
        }
    }
    return return_val;
}

static int is_position_outside(struct position pos) //*outside로 나가면 1, 아니면 0
{
    return (pos.row == -1 || pos.col == -1);
}

static void exit_cs(struct position pos_cur, int start, int dest, int step, struct vehicle_info *vi)
{
    struct position pos_prev = vehicle_path[start][dest][step - 1];

    if (at_crossroad(pos_cur))
    { //*현재위치가 교차로라면.

        if (lock_held_by_current_thread(&vi->map_locks[pos_cur.row][pos_cur.col]))
        { //*지금 쓰레드가 lock을 하고있다면 true를 반환.
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
        }
        // * recursive call
        exit_cs(pos_prev, start, dest, step - 1, vi);
    }
}

//* 교차로 진입할 때 lock걸기!
static int enter_cs(struct position pos_cur, int start, int dest, int step, struct vehicle_info *vi)
{
    //* 다음 위치를 저장.
    struct position pos_next = vehicle_path[start][dest][step + 1];
    //* 다음이 교차로가 아닌 경우.
    if (!at_crossroad(pos_cur))
    {
        //* next not crossroad, all locked!
        return 1;
    }

    if (lock_try_acquire(&vi->map_locks[pos_cur.row][pos_cur.col]))
    { //* 현재 포지션을 lock해주기.

        if (enter_cs(pos_next, start, dest, step + 1, vi))
        {
            return 1;
        }
        else
        {
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
            return 0;
        }
    }
    else
    {
        //* release all
        return 0;
    }
}

//* return 0:termination, 1: move 성공, -1:move 실패
static int try_move(int start, int dest, struct vehicle_info *vi)
{
    struct position pos_cur, pos_next; //*현재위치, 다음위치

    pos_next = vehicle_path[start][dest][vi->step]; //*다음위치 받아오기
    pos_cur = vi->position;                         //*현재위치 받아오기

    if (vi->state == VEHICLE_STATUS_RUNNING)
    { //* 현재 위치가 교차로가 아닐때
        /* check termination */
        if (is_position_outside(pos_next))
        { //*position이 밖이면 -1 ,-1 로 바꿔줌으로써 OUT되게 한다.
            /* actual move */
            vi->position.row = vi->position.col = -1;
            /* release previous */
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]); //*락을 풀어준다. map_locks

            goto end_turn_fail; //*만약 종료되었으면 try_move 는 0을반환.
        }
    }
    if (at_crossroad(pos_cur) && !at_crossroad(pos_next))
    { //*현재 교차로에 있고 다음 step에 교차로를 빠져나간다면.

        lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]); //*일단 자기가 갈곳은 lock

        exit_cs(pos_cur, start, dest, vi->step - 1, vi); //*자기가 지나온곳 lock풀어주기.

        vi->position = pos_next;
        vi->step += 1;

        goto end_turn_success;
    }

    if (at_crossroad(pos_cur) && at_crossroad(pos_next))
    {                            //*현재 교차로에 있고 다음 step에 교차로에 있다면. release 해줄거 없음.
        vi->position = pos_next; //*다음으로 position을 바꿔주기.

        vi->step += 1;

        goto end_turn_success;
    }

    if (!at_crossroad(pos_cur) && at_crossroad(pos_next))
    {                                                                    //*현재는 교차로 아니고 다음에 교차로에 진입할때.
        success_to_lock = enter_cs(pos_next, start, dest, vi->step, vi); //*교차로에 진입하니까 교차로에 대해 lock을 요청.
        if (success_to_lock)
        { //*요청이 받아지면

            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);

            vi->position = pos_next; //* move next position
            vi->step += 1;
        }
        else
        { //*lock을 못한다면 그냥 기다림.
            goto end_turn_block;
        }
        goto end_turn_success;
    }

    //교차로도 아니고 교차로도 아닌곳으로 이동.
    if (lock_try_acquire(&vi->map_locks[pos_next.row][pos_next.col]))
    { //다음
        if (vi->state == VEHICLE_STATUS_READY)
        {                                       // ready 상태인 놈이 있다면
            vi->state = VEHICLE_STATUS_RUNNING; // Running으로 바꿔준다.
        }
        else
        { /* release current position */
            lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
        }
        /* update position */
        vi->position = pos_next; //포지션을 업데이트해주기

        // move한거 check해주기.
        vi->step++;
        goto end_turn_success;
    }
    else
    {
        goto end_turn_block;
    }

    if (start == dest)
    { //*aAA, bBB, cCC
        goto end_turn_fail;
    }

end_turn_success:
    return 1; //그리고 1을 반환
end_turn_fail:
    return 0;
end_turn_block:
    return 2;
}

static struct condition cond;
static struct lock lock;

int count = 0;

void init_on_mainthread(int thread_cnt)
{
    /* Called once before spawning threads */

    cond_init(&cond);
    lock_init(&lock); // monitor 진입을 위한 변수. cond함수들을 호출하기 위해선 lock을 가지고 있어야함.

    vehicles_num = thread_cnt; // thread 개수 가져오기.

    count = 0;
}

void condition_in_while()
{
    lock_acquire(&lock); // * monitor에 접근하기 위해 LOCK을 넘겨줘야한다.

    if ((count == vehicles_num - 1) && (vehicles_num != 1))
    {
        crossroads_step++;
        unitstep_changed();
        cond_broadcast(&cond, &lock);
    }
    else if (vehicles_num == 1)
    {
        crossroads_step++;
        unitstep_changed();
    }
    else
    {
        count++;
        cond_wait(&cond, &lock); //* lock을 먹고 condition에 block될 때 lock을 뱉는다.
        count--;
    }
}

void vehicle_loop(void *_vi) //각 차에대해 thread가 할당됨. 거기에 이제 vehicle_loop 을 계속 돌려주는거임.
{

    int res, i;
    int start, dest;

    struct vehicle_info *vi = _vi; // vehicle_info를 받아온다.

    /*
     * vehicle_info 에 담긴 내용.
     *1. 차이름 ex) a
     *2. state = VEHICLE_STATUS_READY = 0 VEHICLE_STATUS_RUNNING	= 1 VEHICLE_STATUS_FINISHED	= 2이런것들...
     *3. start ex) B =1
     *4. dest ex) C = 2
     *5. position -> 제일 위에 있는 poistion임. 어떻게 움직이는지 저장되어있는 모음집
     *6. lock -> synch.h에 정의되어있음. lock_init  , lock_acquire, lock_try_acquire, lock_release, lock_held_by_current_thread
     */
    start = vi->start - 'A'; //아스키코드라 A를 빼주는거다. 그러면 A는 0, B는 1 , C는 2, D는 3 이런식으로 숫자화 된다!!!!!!!
    dest = vi->dest - 'A';   //아스키코드라 A를 빼주는거다. 그러면 A는 0, B는 1 , C는 2, D는 3 이런식으로 숫자화 된다!!!!!!!

    vi->position.row = vi->position.col = -1; //-1로 일단 초기화시킴.
    vi->state = VEHICLE_STATUS_READY;         // state를 Ready로 초기화시킴.
    vi->step = 0;                             // step도 0으로 초기화시킴.

    while (1)
    {
        /* vehicle main code */
        res = try_move(start, dest, vi); // res는 이제 try_move임. try_move (-1,-1)이면 0을 return. 움직일 수 있으면 1을 리턴.

        if (res == 2)
        {
            thread_yield();
            res = try_move(start, dest, vi);
        }

        condition_in_while();

        if (res == 0)
        {
            vehicles_num -= 1;
            lock_release(&lock);
            break;
        }
        lock_release(&lock);
        thread_yield();
    }

    /* status transition must happen before sema_up */
    vi->state = VEHICLE_STATUS_FINISHED;
}
