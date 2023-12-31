# Cboy

A Gameboy(DMG) emulator written in C, using the SDL2 library.

## Build

You must have the SDL2.0 library installed on your system.

Clone the repository:

```git clone https://github.com/stkrknds/Cboy.git```

Compile, using:

```cd Cboy && make```

The executable is inside the bin folder.
  
## Usage
  
Once you have built the project, you can use the emulator with:

```bin/Cboy rom_file```

## Tests

| Blargg            |    |
|-------------------|----|
| cpu_instrs.gb     | ✅ |
| instr_timing.gb   | ✅ |
| interrupt_time.gb | ❌ |
| mem_timing.gb     | ✅ |
| mem_timing-2.gb   | ✅ |
| oam_bug.gb        | ❌ |
| halt_bug.gb       | ✅ |

| Mooneye/acceptance                 |    |
|------------------------------------|----|
| bits/mem_oam.gb                    | ✅ |
| bits/reg_f.gb                      | ✅ |
| bits/unused_hwio-GS.gb             | ❌ |
| ppu/hblank_ly_scx_timing-GS.gb     | ✅ |
| ppu/intr_2_0_timing.gb             | ✅ |
| ppu/intr_2_mode0_timing_sprites.gb | ❌ |
| ppu/intr_2_mode0_timing.gb         | ✅ |
| ppu/intr_2_mode3_timing.gb         | ✅ |
| ppu/intr_2_oam_ok_timing.gb        | ❌ |
| ppu/lcdon_timing-GS.gb             | ❌ |
| ppu/lcdon_write_timing-GS.gb       | ❌ |
| ppu/stat_irq_blocking.gb           | ✅ |
| ppu/stat_lyc_onoff.gb              | ✅ |
| ppu/vblank_stat_intr-GS.gb         | ✅ |
| instr/daa.gb                       | ✅ |
| interrupts/ie_push.gb              | ✅ |
| oam_dma/basic.gb                   | ✅ |
| oam_dma/reg_read.gb                | ✅ |
| oam_dma/sources-GS.gb              | ❌ |
| timer/div_write.gb                 | ✅ |
| timer/rapid_toggle.gb              | ✅ |
| timer/tim00_div_trigger.gb         | ✅ |
| timer/tim00.gb                     | ✅ |
| timer/tim01_div_trigger.gb         | ✅ |
| timer/tim01.gb                     | ✅ |
| timer/tim10_div_trigger.gb         | ✅ |
| timer/tim10.gb                     | ✅ |
| timer/tim11_div_trigger.gb         | ✅ |
| timer/tim11.gb                     | ✅ |
| timer/tima_reload.gb               | ✅ |
| timer/tima_write_reloading.gb      | ✅ |
| timer/tma_write_reloading.gb       | ✅ |
| add_sp_e_timing.gb                 | ❌ |
| call_timing.gb                     | ❌ |
| call_timing2.gb                    | ❌ |
| call_cc_timing.gb                  | ❌ |
| call_cc_timing2.gb                 | ❌ |
| div_timing.gb                      | ✅ |
| ie_sequence.gb                     | ✅ |
| ei_timing.gb                       | ✅ |
| halt_ime0_ei.gb                    | ✅ |
| halt_ime0_nointr_timing.gb         | ✅ |
| halt_ime1_timing.gb                | ✅ |
| halt_ime1_timing2-GS.gb            | ✅ |
| if_ie_registers.gb                 | ✅ |
| intr_timing.gb                     | ✅ |
| jp_cc_timing.gb                    | ❌ |
| jp_timing.gb                       | ❌ |
| ld_hl_sp_e_timing.gb               | ❌ |
| pop_timing.gb                      | ✅ |
| push_timing.gb                     | ❌ |
| rapid_di_ei.gb                     | ✅ |
| ret_timing.gb                      | ❌ |
    
## Future Work

### CPU

The code for the cpu needs a major refactor. Also, the timing in some instructions is wrong.

### PPU

Need to implement correct sprite fetch delay, lcd timings and DMA blocking.

### APU

Sound implementation. Right now, there is no sound.

### Cartridge

Support more MBC types. Currently, only MBC1 and MBC3 are supported.

## Resources:

1. [Low Level Devel](https://www.youtube.com/watch?v=e87qKixKFME&list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5&pp=iAQB)

2. [GBEDG](https://github.com/Hacktix/GBEDG)

3. [Blargg Tests](https://github.com/retrio/gb-test-roms)

4. [Mooneye Tests](https://github.com/Gekkio/mooneye-test-suite)

5. [Game Boy: Complete Technical Reference](https://github.com/Gekkio/gb-ctr)

6. [DAA](https://ehaskins.com/2018-01-30%20Z80%20DAA/)

7. [TCAGBD](https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf)

8. [Instructions table](https://izik1.github.io/gbops/index.html)

9. [The Ultimate Game Boy Talk (33c3)](https://www.youtube.com/watch?v=HyzD8pNlpwI)
