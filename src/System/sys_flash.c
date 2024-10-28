#include "sys_flash.h"


LOG_MODULE_REGISTER(sys_flash, LOG_LEVEL_DBG);

const struct device *flash_dev ;

enum sys_flash_status sys_flash_erase(uint32_t offset,uint32_t size)
{
     enum sys_flash_status stat = sys_flash_ok;

     uint32_t flashOffset = TEST_PARTITION_OFFSET + (offset);

if (flash_erase(flash_dev, flashOffset, FLASH_PAGE_SIZE) != 0) {
	LOG_ERR("Flash device failed erasing\n");
		stat = sys_flash_err;
        goto common;
}



common:
   return stat;

}
enum sys_flash_status sys_flash_read(uint32_t offset,void *data,uint32_t size)
{
     enum sys_flash_status stat = sys_flash_ok;


          uint32_t flashOffset = TEST_PARTITION_OFFSET + (offset);

if (flash_read(flash_dev, flashOffset, data,size) != 0) {
	LOG_ERR("Flash device failed reading\n");
		stat = sys_flash_err;
        goto common;
}



common:
   return stat;



}
enum sys_flash_status sys_flash_write(uint32_t offset,void *data,uint32_t size)
{
   enum sys_flash_status stat = sys_flash_ok;


     uint32_t flashOffset = TEST_PARTITION_OFFSET + (offset);

sys_flash_erase(offset,size);


if (flash_write(flash_dev, flashOffset, data,size) != 0) {
	LOG_ERR("Flash device failed writing\n");
		stat = sys_flash_err;
        goto common;
}



common:
   return stat;

}
enum sys_flash_status sys_flash_init()
{
   enum sys_flash_status stat = sys_flash_ok;
   flash_dev = TEST_PARTITION_DEVICE;

   LOG_DBG("Partition Flash Initialization");

  	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device not ready\n");
		stat = sys_flash_err;
        goto common;
	}


common:
   return stat;


}