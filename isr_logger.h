/**
 * @file isr_logger.h
 * @brief 高速・スレッドセーフなISR対応遅延評価ロガー
 * @details ISR内では生データとポインタの記録のみを行い、メインスレッドで文字列化を処理する。
 */
#ifndef ISR_LOGGER_H
#define ISR_LOGGER_H

#include "mbed.h"

/**
 * @def ENABLE_ISR_DEBUG
 * @brief デバッグ出力の有効化フラグ（1:有効, 0:無効）
 * @details 0に設定すると関連するすべてのマクロが空処理となり、コンパイル時に完全に削除されます。
 */
#define ENABLE_ISR_DEBUG 1

#if ENABLE_ISR_DEBUG

/**
 * @enum LogTag
 * @brief バッファに格納されるデータの型・種類を示すタグ
 */
enum class LogTag : uint8_t {
    INT,            ///< 整数データ
    FLOAT,          ///< 小数データ
    MSG,            ///< コメント文字列
    WARN,           ///< 警告文字列
    ERR,            ///< エラー文字列
    TELEPLOT_INT,   ///< Teleplot用 整数データ
    TELEPLOT_FLOAT, ///< Teleplot用 小数データ
    ENDL            ///< 改行コマンド
};

/**
 * @struct LogData
 * @brief ログ1件分の情報を保持する構造体 (1要素 12バイト想定)
 */
struct LogData {
    LogTag tag;         ///< データの種類
    const char* str;    ///< 静的文字列へのポインタ (MSG, WARN, ERR, TELEPLOT名用)
    union {
        int i;
        float f;
    } val;              ///< 数値データ
};

/**
 * @brief 内部バッファにデータをプッシュする
 * @warning ユーザーは直接呼び出さず、マクロAPIを使用すること。
 * @param data 記録するログデータ
 */
void isr_log_push(const LogData& data);

/**
 * @brief キュー内のデータをすべてシリアルへ出力する
 * @details メインスレッド（while(1)ループ内など）で定期的に呼び出すこと。
 */
void isr_log_process();

/* =========================================================================
   ユーザー用マクロAPI（ISR内から呼び出し可能）
   ※ 文字列引数 (str_ptr) には、必ず静的な文字列リテラル（"hoge"等）を渡すこと。
   ========================================================================= */
#define LOG_INT(value)            isr_log_push({LogTag::INT, nullptr, {.i = (int)(value)}})
#define LOG_FLOAT(value)          isr_log_push({LogTag::FLOAT, nullptr, {.f = (float)(value)}})
#define LOG_MSG(str_ptr)          isr_log_push({LogTag::MSG, (str_ptr), {.i = 0}})
#define LOG_WARN(str_ptr)         isr_log_push({LogTag::WARN, (str_ptr), {.i = 0}})
#define LOG_ERR(str_ptr)          isr_log_push({LogTag::ERR, (str_ptr), {.i = 0}})
#define LOG_TELEPLOT_INT(n, v)    isr_log_push({LogTag::TELEPLOT_INT, (n), {.i = (int)(v)}})
#define LOG_TELEPLOT_FLOAT(n, v)  isr_log_push({LogTag::TELEPLOT_FLOAT, (n), {.f = (float)(v)}})
#define LOG_ENDL()                isr_log_push({LogTag::ENDL, nullptr, {.i = 0}})

#else // ENABLE_ISR_DEBUG == 0

// 無効化時はすべて空のdo-whileループに置換され、ゼロオーバーヘッドになる
#define LOG_INT(value)            do {} while(0)
#define LOG_FLOAT(value)          do {} while(0)
#define LOG_MSG(str_ptr)          do {} while(0)
#define LOG_WARN(str_ptr)         do {} while(0)
#define LOG_ERR(str_ptr)          do {} while(0)
#define LOG_TELEPLOT_INT(n, v)    do {} while(0)
#define LOG_TELEPLOT_FLOAT(n, v)  do {} while(0)
#define LOG_ENDL()                do {} while(0)
#define isr_log_process()         do {} while(0)

#endif // ENABLE_ISR_DEBUG

#endif // ISR_LOGGER_H
