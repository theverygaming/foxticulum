local reticulum_hdlc_dissector_table = DissectorTable.new("reticulum_hdlc", "Reticulum HDLC dissectors")

local reticulum_hdlc_proto = Proto("reticulum_hdlc", "Reticulum HDLC")
local f_data_len = ProtoField.uint32("reticulum_hdlc.data_length", "Data Length", base.DEC)
local f_data = ProtoField.bytes("reticulum_hdlc.data", "Data")

reticulum_hdlc_proto.fields = { f_data_len, f_data }

-- https://github.com/markqvist/Reticulum/blob/1565126ffd08b9d7bc750ce5df82d5aa3e38183e/RNS/Interfaces/TCPInterface.py#L44-L47
local HDLC_FLAG = 0x7E
local HDLC_ESC = 0x7D
local HDLC_ESC_MASK = 0x20

local function hdlc_unescape(buffer, start_offset, length)
    local unescaped = ByteArray.new()
    local i = 0
    while i < length do
        local b = buffer(start_offset + i, 1):uint()
        if b == HDLC_ESC then
            -- grab another byte
            b = buffer(start_offset + i + 1, 1):uint()
            i = i + 1
            -- XOR it with the escape mask
            b = b ~ HDLC_ESC_MASK
        else
            unescaped:append(ByteArray.new(string.format("%02x", b)))
            i = i + 1
        end
    end
    return unescaped
end

function reticulum_hdlc_proto.dissector(buffer, pinfo, tree)
    local offset = 0
    while offset < buffer:len() do
        -- look for start flag
        local start_pos = nil
        for i = offset, buffer:len() - 1 do
            local b = buffer(i, 1):uint()
            if b == HDLC_FLAG then
                start_pos = i
                break
            end
        end
        -- no start flag? Abort
        if start_pos == nil then
            return buffer:len()
        end

        -- look for the end flag
        local end_pos = nil
        local i = start_pos + 1
        while i < buffer:len() do
            local b = buffer(i, 1):uint()
            if b == HDLC_FLAG then
                end_pos = i
                break
            elseif b == HDLC_ESC then
                -- skip escaped byte
                i = i + 1
            end
            i = i + 1
        end

        -- if there is no end flag for the current HDLC frame we have fragementation and wireshark shall deal with it for us
        if end_pos == nil then
            pinfo.desegment_offset = start_pos
            pinfo.desegment_len = DESEGMENT_ONE_MORE_SEGMENT
            return start_pos
        end

        local frame_len = (end_pos - start_pos) + 1
        local payload_offset = start_pos + 1
        local payload_len = frame_len - 2
        local unescaped_bytes = hdlc_unescape(buffer, payload_offset, payload_len)

        local next_tvb = unescaped_bytes:tvb("HDLC Reticulum Frame")

        local prev_was_tcp = tostring(pinfo.cols.protocol) == "TCP"
        pinfo.cols.protocol = "RNS-HDLC"
        local subtree = tree:add(reticulum_hdlc_proto, buffer(start_pos, frame_len), "Reticulum HDLC")
        subtree:add(f_data_len, unescaped_bytes:len())
         if unescaped_bytes:len() > 0 then
            subtree:add(f_data, next_tvb(0, next_tvb:len()))
        end

        local dissector = reticulum_hdlc_dissector_table:get_dissector(0)
        if dissector ~= nil then
            if prev_was_tcp then
                pinfo.cols.info:set("")
            end
            dissector:call(next_tvb, pinfo, tree)
        end

        offset = end_pos + 1
    end

    return buffer:len()
end

local tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(4242, reticulum_hdlc_proto) -- BackboneInterface
