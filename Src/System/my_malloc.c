#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "my_malloc.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( u32 ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( u32 ) 8 )

#define BYTE_ALIGNMENT 8
#define BYTE_ALIGNMENT_MASK (0x0007)
#define COVERAGE_TEST_MARKER()

/*内存池(64字节对齐)*/
static u8 mem1base[MEM1_MAX_SIZE] __attribute__((aligned (64)));													                //内部SRAM内存池
static u8 mem2base[MEM2_MAX_SIZE]; //__attribute__((aligned (64),section(".ARM.__at_0XC0600000")));					//外部SDRAM内存池,前面2M给LTDC用了(1280*800*2)
static u8 mem3base[MEM3_MAX_SIZE] __attribute__((aligned (64),section(".ARM.__at_0X20000000")));					//内部DTCM内存池
/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

static void prvInsertBlockIntoFreeList( MemType_t memx, BlockLink_t *pxBlockToInsert );
static void prvHeapInit( MemType_t memx );
/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( BYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) BYTE_ALIGNMENT_MASK );

typedef struct _MallocMang
{
  u8 * ucHeap;
  u32 heapSize;
  u32 * pStartAddress;
  BlockLink_t xStart;
  BlockLink_t *pxEnd;
  size_t xFreeBytesRemaining;             //内存堆剩余大小
  size_t xMinimumEverFreeBytesRemaining;  //最小空闲内存块大小
}MallocManger;

MallocManger mallocManger[SRAMBANK];//内关管理器数组
/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/**
  * @brief   将某个内存块插入到空闲内存列表中
  * @param   memx:内存堆类型
  *          pxBlockToInsert:指向内存块的指针
  * @retval  无
  */
static void prvInsertBlockIntoFreeList( MemType_t memx, BlockLink_t *pxBlockToInsert )
{
  BlockLink_t *pxIterator;
  uint8_t *puc;

	/*便利链表，直到找到可以插入的地方*/
	for( pxIterator = &mallocManger[memx].xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/*判断是否可以和前面的内存合并*/
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}else  COVERAGE_TEST_MARKER();

	/*检查是否可以和后面的内存块合并*/
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != mallocManger[memx].pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}else  pxBlockToInsert->pxNextFreeBlock = mallocManger[memx].pxEnd;
	}else  pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;

	/*没有任何内存合并，正常插入*/
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}else  COVERAGE_TEST_MARKER();
}

/**
  * @brief   分配内存
  * @param   memx:内存堆类型
  *          xWantedSize:所需要的大小
  * @retval  指向所分配内存的指针
  */
void *myMalloc(MemType_t memx,u32 xWantedSize)
{
  BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
  void *pvReturn = NULL;

	//关闭中断
	{
		/*如果是首次调用，则需要进行初始化*/
		if( mallocManger[memx].pxEnd == NULL ) prvHeapInit(memx);
		else  COVERAGE_TEST_MARKER();

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/*实际申请内存大小需要加上头部（8字节）*/
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;//加上头部（8字节）
				/*字节对齐*/
				if( ( xWantedSize & BYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( BYTE_ALIGNMENT - ( xWantedSize & BYTE_ALIGNMENT_MASK ) );
					MY_ASSERT( ( xWantedSize & BYTE_ALIGNMENT_MASK ) == 0 );
				}else  COVERAGE_TEST_MARKER();
			}else  COVERAGE_TEST_MARKER();

			if( ( xWantedSize > 0 ) && ( xWantedSize <= mallocManger[memx].xFreeBytesRemaining ) )
			{
				/*从起始地址开始查找链表，直到找到size满足需要的block*/
				pxPreviousBlock = &mallocManger[memx].xStart;
				pxBlock = mallocManger[memx].xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				if( pxBlock != mallocManger[memx].pxEnd )
				{
					/*返回指针指向当前的内存块（需要跳过头部）*/
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/*从链表剔除所分配的内存块*/
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/*如果当前块内存大于所需要的，则将其进行拆分*/
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/*将拆分出来的内存块作为链表插入到内存块链表中*/
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
						MY_ASSERT( ( ( ( size_t ) pxNewBlockLink ) & BYTE_ALIGNMENT_MASK ) == 0 );

						/*重新计算当前内存块尺寸，以便其满足需求*/
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/*将其插入空闲链表中*/
						prvInsertBlockIntoFreeList( memx, pxNewBlockLink );
					}else  COVERAGE_TEST_MARKER();

					mallocManger[memx].xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( mallocManger[memx].xFreeBytesRemaining < mallocManger[memx].xMinimumEverFreeBytesRemaining )
					{
						mallocManger[memx].xMinimumEverFreeBytesRemaining = mallocManger[memx].xFreeBytesRemaining;
					}else  COVERAGE_TEST_MARKER();

					/*将该内存块标记为已用*/
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}else  COVERAGE_TEST_MARKER();
			}else  COVERAGE_TEST_MARKER();
		}else  COVERAGE_TEST_MARKER();
	}
	//开启中断
	MY_ASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) BYTE_ALIGNMENT_MASK ) == 0 );
	return pvReturn;
}

/**
  * @brief   释放内存
  * @param   memx:内存类型
  *          pv:指向内存的指针
  * @retval  无
  */
void myFree(MemType_t memx, void *pv)
{
  uint8_t *puc = ( uint8_t * ) pv;
  BlockLink_t *pxLink;

	if( pv != NULL )
	{
		puc -= xHeapStructSize;   //减去头部，才是该内存块实际的地址
		pxLink = ( void * ) puc;  //防止编译器报错

		/*检查该内存块是否已经被分配过*/
		MY_ASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		MY_ASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/*最高位置0，表示该块已经处于未分配状态*/
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				//禁用中断
				{
					/*将该内存块加入到内存块链表中*/
					mallocManger[memx].xFreeBytesRemaining += pxLink->xBlockSize;
					prvInsertBlockIntoFreeList( memx, ( ( BlockLink_t * ) pxLink ) );
				}
				//启用中断
			}else  COVERAGE_TEST_MARKER();
		}else  COVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

/**
  * @brief   返回内存堆剩余大小
  * @param   memx:内存堆类型
  * @retval  剩余大小
  */
u32 getFreeHeapSize(MemType_t memx)
{
  return mallocManger[memx].xFreeBytesRemaining;
}

/**
  * @brief   返回最小空闲内存块大小
  * @param   memx:内存堆类型
  * @retval  剩余大小
  */
u32 getMinimumEverFreeHeapSize(MemType_t memx)
{
  return mallocManger[memx].xMinimumEverFreeBytesRemaining;
}

/**
  * @brief   初始化堆内存块
  * @param   memx:内存块名称
  * @retval  无
  */
static void prvHeapInit( MemType_t memx )
{
  /*根据堆类型来确定不同的堆大小，以及起始地址*/
  if (memx == MEM_SRAM)
  {
    mallocManger[memx].heapSize = MEM1_MAX_SIZE;
    mallocManger[memx].ucHeap = mem1base;
  }else if (memx == MEM_DRAM)
  {
    mallocManger[memx].heapSize = MEM2_MAX_SIZE;
    mallocManger[memx].ucHeap = mem2base;
  }else
  {
    mallocManger[memx].heapSize = MEM3_MAX_SIZE;
    mallocManger[memx].ucHeap = mem3base;
  } 

  BlockLink_t *pxFirstFreeBlock;
  uint8_t *pucAlignedHeap;      //字节对齐后的起始地址
  size_t uxAddress;
  size_t xTotalHeapSize = mallocManger[memx].heapSize;

	/*保证字节对齐*/
	uxAddress = ( size_t ) mallocManger[memx].ucHeap;
	if( ( uxAddress & BYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( BYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) BYTE_ALIGNMENT_MASK );
		xTotalHeapSize -= uxAddress - ( size_t ) mallocManger[memx].ucHeap;
	}

	pucAlignedHeap = ( uint8_t * ) uxAddress;

	/* xStart为可用内存块链表头*/
	mallocManger[memx].xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	mallocManger[memx].xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd为可用内存块链表尾，放在内存堆尾部*/
	uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~( ( size_t ) BYTE_ALIGNMENT_MASK );
	mallocManger[memx].pxEnd = ( void * ) uxAddress;
	mallocManger[memx].pxEnd->xBlockSize = 0;
	mallocManger[memx].pxEnd->pxNextFreeBlock = NULL;

	/*初始化之后，系统只有一个内存块*/
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = mallocManger[memx].pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	mallocManger[memx].xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	mallocManger[memx].xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}