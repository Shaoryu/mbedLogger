/**
 * @file isr_logger.cpp
 * @brief 高速・スレッドセーフなISR対応遅延評価ロガーの実装
 */
#include "isr_logger.h"

#if ENABLE_ISR_DEBUG

// ビットマスク計算のため、バッファサイズは必ず「2のべき乗」にすること
constexpr size_t LOG_BUFFER_SIZE = 128; 
constexpr uint16_t LOG_BUFFER_MASK = LOG_BUFFER_SIZE - 1;

static LogData log_buffer[LOG_BUFFER_SIZE];
static volatile uint16_t head = 0;
static volatile uint16_t tail = 0;
static volatile bool overflow_flag = false;

void isr_log_push(const LogData& data) {
    // 【極小クリティカルセクション】 Mbed OS標準の軽量排他ロック
    core_util_critical_section_enter();

    log_buffer[head] = data;
    
    // 割り算(%)を使わずビット演算(&)で高速にラップアラウンド計算
    uint16_t next_head = (head + 1) & LOG_BUFFER_MASK;

    if (next_head == tail) {
        // バッファフル：最も古いデータを捨てて新しいデータを守る
        tail = (tail + 1) & LOG_BUFFER_MASK;
        overflow_flag = true;
    }
    head = next_head;

    core_util_critical_section_exit();
}

void isr_log_process() {
    bool local_overflow = false;

    // オーバーフロー判定の読み出しとリセット
    core_util_critical_section_enter();
    if (overflow_flag) {
        local_overflow = true;
        overflow_flag = false;
    }
    core_util_critical_section_exit();

    // あふれ検知時の告知
    if (local_overflow) {
        printf("[OVERFLOW_DETECTED] Some ISR logs were dropped.\n");
    }

    // キューが空になるまで処理を継続
    while (true) {
        LogData d;
        bool has_data = false;

        // データ読み出しのための極小クリティカルセクション
        core_util_critical_section_enter();
        if (head != tail) {
            d = log_buffer[tail];
            tail = (tail + 1) & LOG_BUFFER_MASK;
            has_data = true;
        }
        core_util_critical_section_exit();

        if (!has_data) {
            break; // バッファが空なら終了
        }

        // ここから下はクリティカルセクション外。ISRを一切ブロックせずに時間をかけてシリアル送信する
        switch (d.tag) {
            case LogTag::INT:
                printf("%d ", d.val.i);
                break;
            case LogTag::FLOAT:
                printf("%.3f ", d.val.f); // 用途に合わせて桁数は調整してください
                break;
            case LogTag::MSG:
                printf("%s ", d.str);
                break;
            case LogTag::WARN:
                printf("[WARN] %s ", d.str);
                break;
            case LogTag::ERR:
                printf("[ERROR] %s ", d.str);
                break;
            case LogTag::TELEPLOT_INT:
                printf(">%s:%d\n", d.str, d.val.i);
                break;
            case LogTag::TELEPLOT_FLOAT:
                printf(">%s:%.3f\n", d.str, d.val.f);
                break;
            case LogTag::ENDL:
                printf("\n");
                break;
        }
    }
}

#endif // ENABLE_ISR_DEBUG
