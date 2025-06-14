#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#pragma once

bool create_uinput();
void send_event(int type, int code, int val);
void flush();
void close_uinput();

#endif // __OUTPUT_H__