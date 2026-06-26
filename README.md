
## Kusafuck Programming Language
| Command (記号) | Description (日本語) |
| :---: | :--- |
| `+` | Increment current memory cell value (現在の値を+1) |
| `-` | Decrement current memory cell value (現在の値を-1) |
| `>` | Move data pointer to the right (ポインタを右に移動) |
| `,` | Move data pointer to the left (ポインタを左に移動) |
| `.` | Output current value as UTF-8 encoded text (ASCII文字をUTF-8として出力) |
| `;` | HALT / Immediate termination (プログラムの即時終了) |
| `[` | Jump forward to `]` if current cell is 0 (値が0ならループを抜ける) |
| `]` | Jump backward to `[` if current cell is NOT 0 (値が0以外ならループの先頭に戻る) |
| `%` | Include external library file (外部ファイルの動的インクルード) |
| `{` `}` | Function definition (関数の定義) |
| `/` | Call function dynamically based on memory value (メモリの値に応じた関数の動的呼び出し) |
| `( )` | Comments (コメント) |
| `?` | Show internal instruction help menu (ヘルプメニューの表示) |
| `d` | Yellow colored beautiful memory hex dump (現在のメモリ状態をダンプ表示) |
