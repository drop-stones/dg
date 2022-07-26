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

## 解析の流れ

1. LLVMDependenceGraphBuilder::build()
    1. SVFPointerAnalysis::run()
        1. SVF::AndersenBase::analyze()
  2. LLVMDataDependenceAnalysis::run(): 初期化のみ行い，On-demandで計算
      1. buildGraph()
          1. createDDA()
          2. buildFromLLVM()
          3. buildAllFuns()
          4. buildSubgraph()
          5. buildBBlock()
          6. buildNode()
          7. LLVMReadWriteGraphBuilder::createNode()
              1. createStore
              2. mapPointers
                  1. SVFPointerAnalysis::getLLVMPointsToChecked()
                  2. mapSVFPointsTo()
                      1. class SvfLLVMPointsToSet
                      2. class LLVMPointsToSet(LLVMPointsToImpl *impl = class SvfLLVMPointsToSet)
                  3. LLVMPointsToSet::begin()
                      1. SvfLLVMPointsToSet::get()
                          - `return {_getValue(), Offset::UNKNOWN}`
                  4. DefSite::DefSite(ptrNode, ptr.offset, size)
                  5. return `std::vector<DefSite> result`
      2. dda::DataDependenceAnalysis::run()
      3. MemorySSATransformation::run()
          1. MemorySSATransformation::initialize()
          2. ReadWriteGraph::splitBBlocksOnCalls()
  3. LLVMDependenceGraph::build()
      1. addGlobals
  4. LLVMDependenceGraph::addDefUseEdges()
      1. DataFlowAnalysis::runOnBlock()
      2. LLVMDefUseAnalysis::runOnNode()

### 追加実装の方針

- SVF &rarr; DGへの連携を利用する
  - SVFPointerAnalysis, SvfLLVMPointsToSetなどを改造し，正しい`Offset`を求める
    - SVFPointerAnalysis: SVFの関数を呼ぶwrapperクラス
    - SvfLLVMPointsToSet: SVFのポインタ解析の結果から，LLVMPointerへの変換を担うクラス
      - 現状，`Offset::UNKNOWN`のみを返すテキトー仕様
    - LLVMReadWriteGraphBuilder::mapPointers(): LLVMPointerから，DefSiteを作成
      - 現状，`Offset::UNKNOWN`の場合，`len = Offset::UNKNOWN`となってしまうため(l.120)，意味なし
- DGによる解析後，DefUseエッジを削る
  - LLVMDefUseAnalysisを改造し，不要なDefUseエッジを削る
    - SVFPointerAnalysis::isAlias(): SVFのエイリアス判定
    - 配列アクセスに関して，特別な対処が必要??

### グローバル変数の初期化

処理手順
1. LLVMDataDependenceAnalysis::run() &rarr; buildGraph()
2. LLVMReadWriteGraphBuilder::build()
3. buildFromLLVM() &rarr; buildSubgraph() &rarr; buildBBlock()
4. GraphBuilder::buildNode()
5. LLVMReadWriteGraphBuilder::createNode()
    - グローバル変数に対応するノードの処理
6. DataDependenceAnalysis::run() &rarr; MemorySSATransformation::run() &rarr; initialization()
7. MemorySSATransformation::computeAllDefinitions()
8. MemorySSATransformation::findDefinitions(Use) &rarr; findDefinitionsInBlock()

## 用語

- LVN: Local value numbering
  - 説明: 各命令の操作に対してuniqueなタグを付ける．
  - 用途: 同一タグ(= 同じ操作)である冗長な命令を削除
