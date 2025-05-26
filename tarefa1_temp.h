#ifndef TAREFA1_TEMP_H
#define TAREFA1_TEMP_H

#include "hardware/dma.h"

bool tarefa1_obter_media_temp(dma_channel_config* cfg, int dma_chan);
float tarefa1_termina();

#endif