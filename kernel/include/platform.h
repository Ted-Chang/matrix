#ifndef __PLATFORM_H__
#define __PLATFORM_H__

extern void init_platform();
extern void platform_reboot();
extern void platform_shutdown();
extern void platform_detect_smp();

#endif	/* __PLATFORM_H__ */
