-- pipeline.lua: 用原生 wrk 模拟 HTTP pipelining
-- 原生 wrk 4.2.0 没有 --pipeline 选项，通过环境变量 PIPE_DEPTH 控制每次发送的流水线深度
-- 由 run_test 在 pipeline > 0 时自动附加 -s 参数并设置 PIPE_DEPTH
local depth = tonumber(os.getenv("PIPE_DEPTH") or "1")

request = function()
    local req = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n"
    return req:rep(depth)
end
