#ifndef OS_IDE_H
#define OS_IDE_H

#include <stdbool.h>

bool ide_init();
void ide_irq14_handler();

#endif //OS_IDE_H
