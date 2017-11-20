#include <stdio.h>  
#include <stdlib.h> 
#include "queue.h"
//------------------cache-start--------------------

    /************************************************* 
    Function:       InitQueue 
    Description:    初始化，构造空队列 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         成功返回OK 
    Others:         空队列 queue->front = queue->rear = 0 
    *************************************************/  
    int InitQueue(CircleQueue *queue)  
    {  
        queue->front = queue->rear = 0;  
        queue->count = 0;  
        return OK;  
      
    }  
      
    //判断队列为空和满  
    //1、使用计数器count,队列为空和满时，front都等于rear  
    //2、少用一个元素的空间，约定队列满时：(rear+1)%QUEUESIZE=front,为空时front=rear  
    //rear指向队尾元素的下一个位置，始终为空；队列的长度为(rear-front+QUEUESIZE)%QUEUESIZE  
      
    /************************************************* 
    Function:       IsQueueEmpty 
    Description:    队列是否为空 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         为空返回TRUE，否则返回FALSE 
    Others: 
    *************************************************/  
    int IsQueueEmpty(CircleQueue *queue)  
    {  
        if(queue->count == 0)  
            return TRUE;  
        else  
            return FALSE;  
      
      
    }  
      
    /************************************************* 
    Function:       IsQueueFull 
    Description:    队列是否为满 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         为满返回TRUE，否则返回FALSE 
    Others: 
    *************************************************/  
    int IsQueueFull(CircleQueue *queue)  
    {  
        if(queue->count == QUEUESIZE)  
            return TRUE;  
        else  
            return FALSE;  
      
      
    }  
      
    /************************************************* 
    Function:       EnQueue 
    Description:    入队 
    Input:          队列指针 CircleQueue *queue 
                    数据元素   ElemType e 
    Output: 
    Return:         成功返回OK，失败返回ERROR 
    Others: 
    *************************************************/  
    int EnQueue(CircleQueue *queue, ElemType e)  
    {  
        //验证队列是否已满  
        if(queue->count == QUEUESIZE)  
        {  
            printf("The queue is full");  
            return ERROR;  
        }  
        //入队  
        queue->data[queue->rear] = e;  
        //对尾指针后移  
        queue->rear = (queue->rear + 1) % QUEUESIZE;  
        //更新队列长度  
        queue->count++;  
//printf("e.data=%p\n",*(e.data));
        return OK;  
      
    }  
      
    /************************************************* 
    Function:       DeQueue 
    Description:    出队 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         成功返回数据元素，失败程序退出 
    Others: 
    *************************************************/  
    ElemType DeQueue(CircleQueue *queue)  
    {  
        //判断队列是否为空  
        if(queue->count == 0)  
        {  
            printf("The queue is empty!");  
            exit(EXIT_FAILURE);  
        }  
      
        //保存返回值  
        ElemType e = queue->data[queue->front];  
//free(queue->data[queue->front].data);
        //更新队头指针  
        queue->front = (queue->front + 1) % QUEUESIZE;  
        //更新队列长度  
        queue->count--;  
      
        return e;  
      
    }  
      
    /************************************************* 
    Function:       GetHead 
    Description:    取队头元素 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         成功返回数据元素，否则程序退出 
    Others: 
    *************************************************/  
    ElemType GetHead(CircleQueue *queue)  
    {  
        //判断队列是否为空  
        if(queue->count == 0)  
        {  
            printf("The queue is empty!");  
            exit(EXIT_FAILURE);  
        }  
      
        return queue->data[queue->front];  
      
    }  
      
    /************************************************* 
    Function:       ClearQueue 
    Description:    清空队列 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         成功返回OK 
    Others: 
    *************************************************/  
    int ClearQueue(CircleQueue *queue )  
    {  
        queue->front = queue->rear = 0;  
        queue->count = 0;  
        return OK;  
      
    }  
      
    /************************************************* 
    Function:       GetLength 
    Description:    取得队列的长度 
    Input:          队列指针 CircleQueue *queue 
    Output: 
    Return:         返回队列的长度 
    Others: 
    *************************************************/  
    int GetLength(CircleQueue *queue)  
    {  
        return queue->count;  
           
    }  



