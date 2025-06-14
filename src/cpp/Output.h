#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#pragma once

// Returns true on success, false on failure
bool create_uinput();

// These functions are now thread-safe due to internal mutex
void send_event(int type, int code, int val);
void flush();

void close_uinput();

#endif // __OUTPUT_H__