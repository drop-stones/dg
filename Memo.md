# ポインタ解析/UseDef解析について

## ポインタ解析の判定方法

1. Pointer Subgraph(PS)を作成
    - PS: CFGからポインタ解析に不要なノードを削除した，サブグラフ
    - [PointerGraph.h](https://github.com/mchalupa/dg/blob/master/include/dg/llvm/PointerAnalysis/PointerGraph.h)
    - [PSNode](https://github.com/mchalupa/dg/blob/master/include/dg/PointerAnalysis/PSNode.h)
2. PointerGraph上で，ポインタ解析を実行
    - [LLVMPointerAnalysis](https://github.com/mchalupa/dg/blob/master/include/dg/llvm/PointerAnalysis/PointerAnalysis.h)
      - `(llvm::Value *, Offset)`という情報を保存
    - [PointerAnalysis](https://github.com/mchalupa/dg/blob/master/include/dg/PointerAnalysis/PointerAnalysis.h)
      - `processNode()`において，実際にグラフ上でPoints-to関係の計算を行う
3. Storage Shape Graph(SSG)の情報をPSに追加
    - PAGの制約解決の段階に対応
    - `PSNode->setData()`に，MemoryObjectの情報を追加していく
    - [MemoryObject](https://github.com/mchalupa/dg/blob/0b1ada38d25d34898f5d1e19dbfe6e10006127e6/include/dg/PointerAnalysis/MemoryObject.h)

- LLVMPointerAnalysis: `include/dg/llvm/PointerAnalysis/PointerAnalysis.h`

### 注意

- `UNKNOWN_OFFSET`: 正確なオフセットが計算できない場合に使用
  - 判定: 指し先オブジェクトの範囲を超えるようなオフセットが現れた場合
  - ex) 変数オフセット，範囲外参照

## UseDef解析の判定方法

1. ReadWriteGraphを作成
    - ReadWriteGraph: CFGからデータ読み書き命令のみを抽出
2. 各ノードに，読み書きするメモリ範囲を計算
    - [DefSite](https://github.com/mchalupa/dg/blob/a69378f4f29a63d28cb79e0d6728d069353b19c8/include/dg/ReadWriteGraph/DefSite.h)
3. Reaching Definitions解析
    - [MemorySSA.h](https://github.com/mchalupa/dg/blob/a69378f4f29a63d28cb79e0d6728d069353b19c8/include/dg/MemorySSA/MemorySSA.h): DataAnalysisの中身(`DataAnalysisImpl`)
4. Def-Use解析
    - 到達した定義と，使用ノードを結ぶ
    - [LLVMDefUse](https://github.com/mchalupa/dg/blob/923fc43bb5eff323572d02d323f86cc175b422cc/lib/llvm/DefUse/DefUse.h)
      - `LLVMDependenceGraph::addDefUseEdges()`より呼ばれる

- DefSite: `include/dg/ReadWriteGraph/DefSite.h`
  - `target`: 指し先オブジェクト
  - `offset`: 先頭からのオフセット
  - `len`: 長さ
