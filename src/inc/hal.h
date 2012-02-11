#ifndef __HAL_H__
#define __HAL_H__

void outportb(uint16_t port, uint8_t value);
uint8_t inportb(uint16_t port);
uint16_t inportw(uint16_t port);

#endif	/* __HAL_H__ */
