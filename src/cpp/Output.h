#ifndef __OUTPUT_H__
#define __OUTPUT_H__

extern void send_event(int type, int code, int val);

extern void flush();
extern bool create_uinput(); // Return bool for success/failure
extern void close_uinput();

#endif
