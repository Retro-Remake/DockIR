#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "nec_receive.h"

#define key_power          0x00
#define key_contrast_minus 0x01
#define key_contrast_plus  0x02
#define key_tv_av          0x03
#define key_mute           0x04
#define key_track_last     0x05
#define key_track_next     0x06
#define key_pns            0x07
#define key_vol_up         0x08
#define key_vol_down       0x0b
#define key_skip_fwd       0x09
#define key_skip_back      0x0c
#define key_p_up           0x0a
#define key_p_down         0x0d
#define key_menu           0x0e
#define key_up             0x0f
#define key_l_r            0x10
#define key_left           0x11
#define key_ok             0x12
#define key_right          0x13
#define key_cancel         0x14
#define key_down           0x15
#define key_exit           0x16
#define key_zoom           0x17
#define key_info           0x18
#define key_timer          0x19
#define key_pause          0x1a
#define key_play           0x1b
#define key_stop           0x1c
#define key_eject          0x1d
#define key_1              0x1e
#define key_2              0x1f
#define key_3              0x20
#define key_4              0x21
#define key_5              0x22
#define key_6              0x23
#define key_7              0x24
#define key_8              0x25
#define key_9              0x26
#define key_10_plus        0x27
#define key_0              0x28
#define key_repeat         0x29
/* 超时释放：最后一次收到 IR 帧后多久释放
 * NEC repeat 帧间隔约 110ms，设 200ms 可容纳 1 次丢帧 */
#define RELEASE_TIMEOUT_MS  200

/* PIO 检测到 NEC repeat code 时 push 到 FIFO 的特殊标记 */
#define NEC_REPEAT_MARKER   0xFFFFFFFF

// Assignable HID_KEY values can be found at:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
static uint8_t ir_to_hid(uint8_t rx_data) {
  switch (rx_data) {
    case key_power:          return HID_KEY_NONE;
    case key_contrast_plus:  return HID_KEY_M;
    case key_contrast_minus: return HID_KEY_T;
    case key_tv_av:          return HID_KEY_D;
    case key_mute:           return HID_KEY_MUTE;
    case key_skip_fwd:       return HID_KEY_F;
    case key_skip_back:      return HID_KEY_R;
    case key_pns:            return HID_KEY_V;

    case key_vol_up:         return HID_KEY_VOLUME_UP;
    case key_track_next:     return HID_KEY_N;
    case key_p_up:           return HID_KEY_PAGE_UP;
    case key_vol_down:       return HID_KEY_VOLUME_DOWN;
    case key_track_last:     return HID_KEY_P;
    case key_p_down:         return HID_KEY_PAGE_DOWN;

    // Navigation cluster
    case key_menu:   return HID_KEY_F12;
    case key_up:     return HID_KEY_ARROW_UP;
    case key_l_r:    return HID_KEY_TAB;
    case key_left:   return HID_KEY_ARROW_LEFT;
    case key_ok:     return HID_KEY_ENTER;
    case key_right:  return HID_KEY_ARROW_RIGHT;
    case key_cancel: return HID_KEY_BACKSPACE;
    case key_down:   return HID_KEY_ARROW_DOWN;
    case key_exit:   return HID_KEY_ESCAPE;

    case key_zoom:  return HID_KEY_G;
    case key_info:  return HID_KEY_S;
    case key_timer: return HID_KEY_NONE;

    case key_pause: return HID_KEY_SPACE;
    case key_play:  return HID_KEY_A;
    case key_stop:  return HID_KEY_B;
    case key_eject: return HID_KEY_C;

    // Numpad
    case key_1:       return HID_KEY_1;
    case key_2:       return HID_KEY_2;
    case key_3:       return HID_KEY_3;
    case key_4:       return HID_KEY_4;
    case key_5:       return HID_KEY_5;
    case key_6:       return HID_KEY_6;
    case key_7:       return HID_KEY_7;
    case key_8:       return HID_KEY_8;
    case key_9:       return HID_KEY_9;
    case key_10_plus: return HID_KEY_MINUS;
    case key_0:       return HID_KEY_0;
    case key_repeat:  return HID_KEY_NONE;

    default: return 0;
  }
}

/* 发送一次 key-down + key-up tap，确保两个 HID 报告都被 USB 主机收到 */
static void send_hid_tap(uint8_t hid_key)
{
  if (!tud_hid_n_ready(0)) return;

  uint8_t kcode[6] = { hid_key, 0, 0, 0, 0, 0 };
  tud_hid_n_keyboard_report(0, 0, 0, kcode);       /* key-down */

  /* 等待 key-down 报告被主机取走，endpoint 重新就绪 */
  uint32_t t0 = board_millis();
  tud_task();
  while (!tud_hid_n_ready(0)) {
    tud_task();
    if (board_millis() - t0 > 50) break;            /* 超时保护 */
  }

  sleep_ms(80);

  tud_hid_n_keyboard_report(0, 0, 0, NULL);         /* key-up */
}


int main(void)
{
  stdio_init_all();
  board_init();
  tusb_init();

  /* LED 用于诊断：收到 repeat 帧时闪烁 */
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 0);

  PIO pio = pio0;
  uint rx_gpio = 27;
  int rx_sm = nec_rx_init (pio, rx_gpio);
  uint8_t rx_address, rx_data;

  uint8_t last_hid_key = 0;      /* 当前按住的 HID 键码 */
  bool active = false;            /* 是否有按键处于活动状态 */
  uint32_t last_ir_time = 0;     /* 上次收到任何 IR 帧的时间 */
  uint32_t keepCount = 0;

  while (1)
  {
    tud_task();
    uint32_t now = board_millis();

    /* === 1. 超时释放 === */
    if (active && (now - last_ir_time >= RELEASE_TIMEOUT_MS)) {
      active = false;
      last_hid_key = 0;
      keepCount = 0;
      gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }

    /* === 2. 处理 PIO FIFO 红外帧 === */
    while (!pio_sm_is_rx_fifo_empty(pio, (uint)rx_sm)) {
      uint32_t rx_frame = pio_sm_get(pio, (uint)rx_sm);

      if (rx_frame == NEC_REPEAT_MARKER) {
        /* NEC repeat code：遥控器长按时每 ~110ms 收到一次 */

        if (active && last_hid_key ) {
          last_ir_time = now;
          if(keepCount>0){
            gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
            send_hid_tap(last_hid_key);
          }
        }
        keepCount=1;
      } else if (nec_decode_frame(rx_frame, &rx_address, &rx_data)) {
        /* 完整 NEC 帧（首次按键 或 部分遥控器长按也重发完整帧） */
        uint8_t hid_key = ir_to_hid(rx_data);
        if (hid_key) {
          last_hid_key = hid_key;
          last_ir_time = now;
          keepCount = 0;
          active = true;
          gpio_put(PICO_DEFAULT_LED_PIN, 1);
          send_hid_tap(hid_key);
        }
        /* 未映射的按键直接忽略，不影响当前活动状态 */
      }
      /* 无效帧直接忽略，不影响当前活动状态 */
    }
  }
  return 0;
}


uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}
