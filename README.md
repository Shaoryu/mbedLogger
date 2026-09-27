# mbedLogger

```sample.cpp
#include "mbed.h"
#include "isr_logger.h"

// 割り込み用のタイマー
Ticker control_ticker;

// テスト用のダミー変数
float motor_speed = 0.0f;
int isr_counter = 0;

/**
 * @brief タイマー割り込みハンドラ (ISR)
 * @details 10ms周期で実行される。ここでは重い処理(printfなど)は一切行わない。
 */
void control_isr() {
    isr_counter++;
    motor_speed += 0.5f;
    if (motor_speed > 100.0f) motor_speed = 0.0f;

    // 1. Teleplot用のログ (名前と変数をパッケージ化、自動で改行される)
    LOG_TELEPLOT_FLOAT("Speed", motor_speed);

    // 2. 複数のデータを横に並べて出力する通常のログ
    LOG_MSG("Ticks:");
    LOG_INT(isr_counter);
    LOG_ENDL(); // ここで改行

    // 3. 特定の条件で警告やエラーを出すテスト
    if (isr_counter % 50 == 0) {
        LOG_WARN("Counter reached a multiple of 50");
        LOG_ENDL();
    }
}

int main() {
    printf("--- System Start ---\n");

    // 10ms (10000us) 周期で割り込みを開始
    control_ticker.attach(&control_isr, 10ms);

    while (true) {
        // -----------------------------------------------------
        // 1. メインループの先頭（あるいは余裕のあるタイミング）で
        //    バッファに溜まったISRのログを一気にPCへ送信する
        // -----------------------------------------------------
        isr_log_process();

        // -----------------------------------------------------
        // 2. メインスレッド側のその他の処理
        //    (ネットワーク通信、LCD描画など時間がかかる処理のシミュレート)
        // -----------------------------------------------------
        // この100msの間に、ISRは10回呼び出され、バッファにデータが蓄積される
        ThisThread::sleep_for(100ms);
    }
}
```
