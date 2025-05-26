/**
 * ------------------------------------------------------------
 *  Arquivo: tarefa1_temp.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Este módulo implementa a Tarefa 1 do executor cíclico,
 *      responsável por realizar a leitura do sensor interno
 *      de temperatura utilizando ADC + DMA, durante um intervalo
 *      contínuo de 0,5 segundos.
 *
 *      A leitura é feita em blocos menores (10.000 amostras)
 *      para evitar o uso excessivo da SRAM do RP2040. Cada
 *      bloco é transferido via DMA e processado de forma
 *      incremental para obter a média da temperatura ao final
 *      do intervalo total.
 *
 *  Funcionalidades:
 *      - Converte valores brutos do ADC para graus Celsius.
 *      - Controla o tempo de aquisição com precisão usando
 *        o clock interno via 'get_absolute_time()'.
 *      - Utiliza DMA canal 0 e depende da flag 'dma_temp_done'
 *        sinalizada pelo handler definido em 'irq_handlers.c'.
 *
 *  Relacionamento:
 *      - Chamado pelo laço principal em 'main.c' como tarefa do ciclo.
 *      - Requer configuração do canal DMA e IRQ em 'setup.c'.
 *
 *  
 *  Data: 11/05/2025
 * ------------------------------------------------------------
 */

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "tarefa1_temp.h"

#define BLOCO_AMOSTRAS 10000
#define DURACAO_AMOSTRAGEM_US 500000  // 0,5 segundos em microssegundos

static uint16_t buffer_temp[BLOCO_AMOSTRAS];
extern volatile bool dma_temp_done;

/**
 * @brief Converte valor do ADC para temperatura em °C.
 * 
 * @param raw Valor bruto de 12 bits lido do ADC.
 * @return float Temperatura em graus Celsius.
 */
static float convert_to_celsius(uint16_t raw) {
    const float conv = 3.3f / (1 << 12);  // Conversão para tensão
    float voltage = raw * conv;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

/**
 * @brief Inicia uma transferência via DMA de um bloco do sensor de temperatura.
 *
 * @param buffer Buffer de destino.
 * @param cfg Configuração do canal DMA.
 * @param dma_chan Canal DMA utilizado.
 */
static void iniciar_dma_temp(uint16_t *buffer, dma_channel_config *cfg, int dma_chan) {
    adc_select_input(4);           // Canal 4 → sensor interno
    adc_fifo_drain();
    adc_run(false);
    adc_fifo_setup(true, true, 1, false, false);
    adc_run(true);

    dma_channel_configure(
        dma_chan,
        cfg,
        buffer, &adc_hw->fifo,
        BLOCO_AMOSTRAS,
        true
    );
}

/**
 * @brief Executa a Tarefa 1 do executor cíclico: coleta de temperatura por 0,5s.
 *
 * @param cfg_temp Configuração do canal DMA.
 * @param dma_chan Número do canal DMA utilizado.
 * @return float Temperatura média calculada ao final do intervalo.
 */

typedef enum {
    ESTADO_PARADO = 0,
    ESTADO_INICIANDO,
    ESTADO_AGUARDANDO_DMA
} estado_tarefa1_t;

static estado_tarefa1_t estado_t1 = ESTADO_PARADO;
static absolute_time_t inicio_amostragem;
static float soma_temp = 0.0f;
static uint32_t total_amostras = 0;
static float media_temp = 0.0f;

bool tarefa1_obter_media_temp(dma_channel_config* cfg_temp, int dma_chan) {
    switch (estado_t1) {
        case ESTADO_PARADO:
            soma_temp = 0.0f;
            total_amostras = 0;
            inicio_amostragem = get_absolute_time();
            estado_t1 = ESTADO_INICIANDO;
            break;

        case ESTADO_INICIANDO:
            dma_temp_done = false;
            iniciar_dma_temp(buffer_temp, cfg_temp, dma_chan);
            estado_t1 = ESTADO_AGUARDANDO_DMA;
            break;

        case ESTADO_AGUARDANDO_DMA:
            if (dma_temp_done) {
                adc_run(false);  // Desliga adc
                for (int i = 0; i < BLOCO_AMOSTRAS; i++) {
                    soma_temp += convert_to_celsius(buffer_temp[i]);
                }
                total_amostras += BLOCO_AMOSTRAS;

                if (absolute_time_diff_us(inicio_amostragem, get_absolute_time()) >= DURACAO_AMOSTRAGEM_US) {
                    media_temp = soma_temp / total_amostras;
                    estado_t1 = ESTADO_PARADO;
                    return true;  // Ciclo finalizado
                } else {
                    estado_t1 = ESTADO_INICIANDO;  // Novo bloco
                }
            }
            break;
    }
    return false;  // Ciclo ainda em andament
}

float tarefa1_termina(){
    return media_temp;
}