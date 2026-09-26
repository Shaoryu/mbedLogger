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
    // 【極小クリティカルセクション】
    core_util_critical_section_enter();

    log_buffer[head] = data;
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

    // 出力結合用のローカルバッファ (サイズは用途に応じて調整してください)
    char out_buf[512];
    size_t offset = 0;
    out_buf[0] = '\0';

    if (local_overflow) {
        offset += snprintf(out_buf + offset, sizeof(out_buf) - offset, "[OVERFLOW_DETECTED] Some ISR logs were dropped.\n");
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
            break; // バッファが空ならループ終了
        }

        // 【安全対策】バッファの残り容量が少ない場合は、一度フラッシュして空にする
        // (長い文字列が来た場合に備えて64バイト程度の余裕を持たせる)
        if (sizeof(out_buf) - offset < 64) {
            printf("%s", out_buf);
            offset = 0;
            out_buf[0] = '\0';
        }

        // バッファの末尾(out_buf + offset)に文字列を追記していく
        int written = 0;
        switch (d.tag) {
            case LogTag::INT:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "%d ", d.val.i);
                break;
            case LogTag::FLOAT:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "%.3f ", d.val.f);
                break;
            case LogTag::MSG:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "%s ", d.str);
                break;
            case LogTag::WARN:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "[WARN] %s ", d.str);
                break;
            case LogTag::ERR:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "[ERROR] %s ", d.str);
                break;
            case LogTag::TELEPLOT_INT:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, ">%s:%d\n", d.str, d.val.i);
                break;
            case LogTag::TELEPLOT_FLOAT:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, ">%s:%.3f\n", d.str, d.val.f);
                break;
            case LogTag::ENDL:
                written = snprintf(out_buf + offset, sizeof(out_buf) - offset, "\n");
                break;
        }

        // 書き込んだ文字数分だけオフセットを進める
        if (written > 0) {
            // 万が一バッファ上限に達した場合のフェイルセーフ
            size_t max_writable = sizeof(out_buf) - offset - 1;
            offset += (static_cast<size_t>(written) < max_writable) ? written : max_writable;
        }
    }

    // 最後に残っている文字列を1回のprintfで出力
    if (offset > 0) {
        printf("%s", out_buf);
    }
}

#endif // ENABLE_ISR_DEBUG
