#ifndef __CFGSAVE_H__
#define __CFGSAVE_H__

#include "main.h"

#define save_byte 100


void cfgSave();
void cfgRead();
void save_begin(void);
void cfgsave_thread_entry(void *parameter);


#endif /* __CFGSAVE_H__ */
