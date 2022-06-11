#include <linux/module.h>

static int __init pcd_platform_driver_init(void){
	printk("Hii\n");
	return 0;
}

static void __exit pcd_platform_driver_cleanup(void) {
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YC Lin");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n platform pcdevs");
