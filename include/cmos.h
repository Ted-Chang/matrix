#ifndef __CMOS_H__
#define __CMOS_H__

#define RTC_INDEX	0x70	/* Bit 7 = NMI enable(1) / disable(0)
				 * bits 0..6 index
				 */

#define RTC_IO		0x71	/* Data register,
				 * NOTE: the operation following a write to
				 * RTC_INDEX should be an access (read or write)
				 * to RTC_IO
				 */


#define RTC_SEC		0x00	/* Seconds register */
#define RTC_SEC_ALRM	0x01	/* Seconds register for alarm */
#define RTC_MIN		0x02	/* Minutes register */
#define RTC_MIN_ALRM	0x03	/* Minutes register for alarm */
#define RTC_HOUR	0x04	/* Hours register */
#define RTC_HOUR_ALRM	0x05	/* Hours register for alarm */
#define RTC_WDAY	0x06	/* Day of the week, 1..7, Sunday = 1 */
#define RTC_MDAY	0x07	/* Day of the month, 1..31 */
#define RTC_MONTH	0x08	/* Month 1..12 */
#define RTC_YEAR	0x09	/* Year 0..99 */
#define RTC_REG_A	0x0A
#define RTC_A_UIP	0x80	/* Update in progress. When clear, no update
				 * until occur for 244 micro seconds.
				 */
#define RTC_A_DV	0x70	/* Divider bits */
#define RTC_A_DV_OK	0x20	/* Normal */
#define RTC_A_DV_STOP	0x70	/* Stop, a re-start starts halfway through a
				 * cycle, i.e. the update occurs after 500ms
				 */

#define RTC_A_RS	0x0F	/* Int. freq */
#define RTC_A_RS_DEF	6	/* Default freq */
#define RTC_REG_B	0x0B
#define RTC_B_SET	0x80	/* Inhibit updates */
#define RTC_B_PIE	0x40	/* Enable periodic interrupts */
#define RTC_B_AIE	0x20	/* Enable alarm interrupts */
#define RTC_B_UIE	0x10	/* Enable update ended interrupts */
#define RTC_B_SQWE	0x08	/* Enable square wave output */
#define RTC_B_DM_BCD	0x04	/* Data is in BCD (otherwise binary) */
#define RTC_B_24	0x02	/* Count hours in 24-hours mode */
#define RTC_B_DSE	0x01	/* Automatic (wrong) daylight savings updates */

#define RTC_REG_C	0x0C

/* Contents of the general purpose CMOS RAM (source IBM reference mail) */
#define CMOS_STATUS	0x0E
#define CMOS_LOST_POWER	0x80	/* Chip out of power */
#define CMOS_BAD_CHKSUM	0x40	/* Checksum is incorrect */
#define CMOS_BAD_CONFIG	0x20	/* Bad configuration info */
#define CMOS_BAD_MEMSIZE 0x10	/* Wrong memory size of CMOS */
#define CMOS_BAD_HD	0x08	/* Harddisk failed */
#define CMOS_BAD_TIME	0x04	/* CMOS time is invalid */
				/* bits 0 and 1 are reserved */

#endif	/* __CMOS_H__ */
