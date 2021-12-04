# NVIDIA Image Scaling plugin for UE4.27.1
超解像ライブラリ NVIDIA Image Scaling (NIS)のUE4.27.1用プラグインです。   
UE4.26以前では利用することができません。

元となる NIS SDK は以下。   
https://github.com/NVIDIAGameWorks/NVIDIAImageScaling

# ビルド方法
1. ソースコードをダウンロードします
1. エンジンプラグインとしてUE4のソースコードに追加します
    1. Engine/Plugins/Runtime/Nvidia フォルダに ImageScaling フォルダをコピーします
1. エンジンをビルドし直します
    1. エンジンのソリューション・プロジェクトの更新をお忘れなく
1. ビルドが完了したらエディタを起動し、Image Scalingプラグインを有効化します
1. 再起動をするとNISが利用できるようになります

# CVars
## r.NVIDIA.NIS.Enabled
NISの有効・無効を設定します。   
デフォルトは無効 (0) です。   
FSRプラグインを利用している場合、FSRを無効にしてから有効にしてください。

## r.NVIDIA.NIS.Sharpness
NISのシャープネスを 0.0～1.0 の範囲で設定します。
デフォルトは 0.5 です。
