/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Ciclo principal do sistema embarcado, baseado em um
 *      executor cíclico com 3 tarefas principais:
 *
 *      Tarefa 1 - Leitura da temperatura via DMA (meio segundo)
 *      Tarefa 2 - Exibição da temperatura e tendência no OLED
 *      Tarefa 3 - Análise da tendência da temperatura
 *
 *      O sistema utiliza watchdog para segurança, terminal USB
 *      para monitoramento e display OLED para visualização direta.
 *
 *  
 *  Data: 12/05/2025
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "setup.h"
#include "tarefa1_temp.h"
#include "tarefa2_display.h"
#include "tarefa3_tendencia.h"
#include "tarefa4_controla_neopixel.h"
#include "neopixel_driver.h"
#include "testes_cores.h"  
#include "pico/stdio_usb.h"




float media;
tendencia_t t;
volatile bool leitura_temp_concluida = false;
absolute_time_t ini_tarefa1, fim_tarefa1, ini_tarefa2, fim_tarefa2, ini_tarefa3, fim_tarefa3, ini_tarefa4, fim_tarefa4;
struct repeating_timer timer1, timer2, timer3, timer4, timer5;


/*******************************/
bool tarefa_1(struct repeating_timer *unused){
// --- Tarefa 1: Leitura de temperatura via DMA ---
        static bool ciclo_finalizado = false;

    if (!ciclo_finalizado) {
        ciclo_finalizado = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL);
    } else {
        media = tarefa1_termina();
        ciclo_finalizado = false;

        printf("Temperatura: %.2f °C\n", media);

        if (!leitura_temp_concluida) {
            leitura_temp_concluida = true;
            printf(">> Primeira leitura concluída. Tarefas 2 a 5 liberadas.\n");
        }
    }

    return true;
}
/*******************************/
bool tarefa_2(struct repeating_timer *unused)
{
    // --- Tarefa 3: Análise da tendência térmica ---
        static int contador = 0;
    if (!leitura_temp_concluida) return true;

    if (++contador < 3) return true;
    contador = 0;

    ini_tarefa3 = get_absolute_time();
    t = tarefa3_analisa_tendencia(media);
    fim_tarefa3 = get_absolute_time();

    int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
    printf("Tarefa 2: Tendência → %s | T3: %.3fs\n", tendencia_para_texto(t), tempo3_us / 1e6);
    return true;
}
/*******************************/
bool tarefa_3(struct repeating_timer *unused)
{
        // --- Tarefa 2: Exibição no OLED ---
        static int contador = 0;
    if (!leitura_temp_concluida) return true;

    if (++contador < 3) return true;
    contador = 0;

    ini_tarefa2 = get_absolute_time();
    tarefa2_exibir_oled(media, t);
    fim_tarefa2 = get_absolute_time();

    int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
    printf("Tarefa 3: Display OLED | T2: %.3fs\n", tempo2_us / 1e6);
    return true;
}
/*******************************/
bool tarefa_4(struct repeating_timer *unused)
{
// --- Tarefa 4: Cor da matriz NeoPixel por tendência ---
        static int contador = 0;
    if (!leitura_temp_concluida) return true;

    if (++contador < 3) return true;
    contador = 0;

    ini_tarefa4 = get_absolute_time();
    tarefa4_matriz_cor_por_tendencia(t);
    fim_tarefa4 = get_absolute_time();

    int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4);
    printf("Tarefa 4: NeoPixel | T4: %.3fs\n", tempo4_us / 1e6);
    return true;
}
bool tarefa_5(struct repeating_timer *unused)
{
// --- Tarefa 5: Extra ---
        static int contador = 0;
    static bool estado = false;

    if (!leitura_temp_concluida) return true;

    if (++contador < 3) return true;
    contador = 0;

    if (media < 1.0f) {
        if (estado) {
            npClear();
        } else {
            npSetAll(COR_BRANCA);
        }
        npWrite();
        estado = !estado;
    } else {
        npClear();
        npWrite();
    }

    return true;
}

int main() {
    setup();  // Inicializações: ADC, DMA, interrupções, OLED, etc.

   // while (!stdio_usb_connected()) {
   //     sleep_ms(100);
   // }

    //Ativa o watchdog com timeout de 2 segundos
    //watchdog_enable(2000, 1);

    while (true) {
         setup();

    // Aguarda conexão usb para printf funcionar corretamente
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // Desativado por segurança durante testes
    // Watchdog_enable(2000, 1);

    // Timers
    add_repeating_timer_ms(500, tarefa_1, NULL, &timer1);
    add_repeating_timer_ms(500, tarefa_2, NULL, &timer2);
    add_repeating_timer_ms(500, tarefa_3, NULL, &timer3);
    add_repeating_timer_ms(500, tarefa_4, NULL, &timer4);
    add_repeating_timer_ms(500, tarefa_5, NULL, &timer5);

    while (true) {
        // Watchdog_update(); se watchdog estiver habilitado
        tight_loop_contents();
    }

    return 0;
}
}