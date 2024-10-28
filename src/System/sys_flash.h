#ifndef _SYS_FLASH_H_
#define _SYS_FLASH_H_


#include "app_defines.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

enum sys_flash_status {
  sys_flash_ok = 0,
  sys_flash_err
};

#define TEST_PARTITION	storage_partition
#define TEST_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(TEST_PARTITION)


#define FLASH_PAGE_SIZE 4096


#define Priv_key 0x00
#define Priv_Key_Size 16

extern enum sys_flash_status sys_flash_erase(uint32_t offset,uint32_t size);
extern enum sys_flash_status sys_flash_read(uint32_t offset,void *data,uint32_t size);
extern enum sys_flash_status sys_flash_write(uint32_t offset,void *data,uint32_t size);
extern enum sys_flash_status sys_flash_init();



#endif