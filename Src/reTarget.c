/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-12-23 16:18:29
 * @LastEditors: PanHan
 * @LastEditTime: 2019-12-23 17:49:18
 */
#include "stdio.h"
#include <stdlib.h>
#include "sys.h"
int _write(int fd, char *ptr, int len)
{
    int i = 0;

    /*
     * write "len" of char from "ptr" to file id "fd"
     * Return number of char written.
     *
    * Only work for STDOUT, STDIN, and STDERR
     */
    if (fd > 2)
    {
        return -1;
    }

    while (*ptr && (i < len))
    {
        while((USART1->ISR & 0X40) == 0);//循环发送,直到发送完毕   
        USART1->TDR = (u8)*ptr;      
        if (*ptr == '\n')
        {
            USART1->TDR = '\r';;
        }

        i++;
        ptr++;
    }

    return i;
}