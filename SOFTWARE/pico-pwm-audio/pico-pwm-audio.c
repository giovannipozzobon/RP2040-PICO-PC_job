#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/clocks.h" //jobond 13/06/2025 Inserito perchè mancava e dava errore hardware/structs/clocks.h
 
// Audio PIN is to match some of the design guide shields. 
#define AUDIO_PIN1 27  // you can change this to whatever you like
#define AUDIO_PIN2 28  // you can change this to whatever you like

/* 
 * This include brings in static arrays which contain audio samples. 
 * if you want to know how to make these please see the python code
 * for converting audio samples into static arrays. 
 */
#include "sample.h"
//#include "ring.h"
//#include "thats_cool.h"
int wav_position = 0;

/*
 * PWM Interrupt Handler which outputs PWM level and advances the 
 * current sample. 
 * 
 * We repeat the same value for 8 cycles this means sample rate etc
 * adjust by factor of 8   (this is what bitshifting <<3 is doing)
 * 
 */
void pwm_interrupt_handler() {
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN1));    
    //pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN2));    
    if (wav_position < (WAV_DATA_LENGTH<<3) - 1)
    { 
        // set pwm level 
        // allow the pwm value to repeat for 8 cycles this is >>3 
        pwm_set_gpio_level(AUDIO_PIN1, WAV_DATA[wav_position>>3]);  
        pwm_set_gpio_level(AUDIO_PIN2, WAV_DATA[wav_position>>3]);  
        wav_position++;
    }
    else
    {
        // reset to start
        wav_position = 0;
    }
}

int main(void) {
    /* Overclocking for fun but then also so the system clock is a 
     * multiple of typical audio sampling rates.
     */
    stdio_init_all();
    set_sys_clock_khz(176000, true); 
    gpio_set_function(AUDIO_PIN1, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PIN2, GPIO_FUNC_PWM);

    int audio_pin_slice1 = pwm_gpio_to_slice_num(AUDIO_PIN1);
    int audio_pin_slice2 = pwm_gpio_to_slice_num(AUDIO_PIN2);

    // Setup PWM interrupt to fire when PWM cycle is complete
    pwm_clear_irq(audio_pin_slice1);
    //pwm_clear_irq(audio_pin_slice2);
    pwm_set_irq_enabled(audio_pin_slice1, true);
    //pwm_set_irq_enabled(audio_pin_slice2, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler); 
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();
    /* Base clock 176,000,000 Hz divide by wrap 250 then the clock divider further divides
     * to set the interrupt rate. 
     * 
     * 11 KHz is fine for speech. Phone lines generally sample at 8 KHz
     * 
     * 
     * So clkdiv should be as follows for given sample rate
     *  8.0f for 11 KHz1
     *  4.0f for 22 KHz
     *  2.0f for 44 KHz etc
     */
    pwm_config_set_clkdiv(&config, 8.0f); 
    pwm_config_set_wrap(&config, 250); 
    pwm_init(audio_pin_slice1, &config, true);
    pwm_init(audio_pin_slice2, &config, true);

    pwm_set_gpio_level(AUDIO_PIN1, 0);
    pwm_set_gpio_level(AUDIO_PIN2, 0);

    while(1) {
        __wfi(); // Wait for Interrupt
    }
}
