// 必须先定义Module，再importScripts，否则onRuntimeInitialized不触发
self.Module = {
    onRuntimeInitialized: function() {
        self.postMessage({type: "ready"});
    }
};

// 加载WASM胶水代码
importScripts("./out.js");

self.onmessage = function(e) {
    const msg = e.data;
    if (msg.type !== "ai_calc") return;

    const {boardArr,turn,sum0,sum1} = msg;

    // 分配WASM内存，拷贝棋盘数据
    const ptr = Module._malloc(64 * 4);
    for(let i=0;i<64;i++){
        Module.setValue(ptr + i*4, boardArr[i], 'i32');
    }

    // 调用C++ AI计算
    const aiPos = Module.ccall(
        "worker_ai_calc",
        "number",
        ["number","number","number","number"],
        [ptr, turn, sum0, sum1]
    );

    Module._free(ptr);

    // 返回结果：aiPos 0=pass，1-64为落子位置
    self.postMessage({
        type: "ai_result",
        aiPos: aiPos
    });
};
