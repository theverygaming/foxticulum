reticulum_proto = Proto("reticulum", "Reticulum Protocol")

local f_flags = ProtoField.uint8("reticulum.flags", "Flags", base.HEX)
local f_flag_packet_type = ProtoField.uint8("reticulum.flags.ptype", "Packet Type", base.DEC, {[0] = "Data", [1] = "Announce", [2] = "Link Request", [3] = "Proof"}, 0x03)
local f_flag_destination_type = ProtoField.uint8("reticulum.flags.dtype", "Destination Type", base.DEC, {[0] = "Single", [1] = "Group", [2] = "Plain", [3] = "Link"}, 0x03 << 2)
local f_flag_propagation_type = ProtoField.uint8("reticulum.flags.proptype", "Propagation Type", base.DEC, {[0] = "Broadcast", [1] = "Transport"}, 0x01 << 4)
local f_flag_context = ProtoField.bool("reticulum.flags.context", "Context", 8, {"Set", "Not set"}, 0x1 << 5)
local f_flag_header_type = ProtoField.uint8("reticulum.flags.htype", "Header Type", base.DEC, {[0] = "Without Transport Destination", [1] = "Transport Destination"}, 0x01 << 6)
local f_flag_ifac = ProtoField.bool("reticulum.flags.ifac", "Context", 8, {"Set", "Not set"}, 0x1 << 7)
local f_hops = ProtoField.uint8("reticulum.hops", "Hops", base.DEC)
local f_transport_dest = ProtoField.bytes("reticulum.transport_dest", "Transport Destination", base.NONE)
local f_dest = ProtoField.bytes("reticulum.dest", "Destination", base.NONE)
local f_context = ProtoField.uint8("reticulum.context", "Context", base.HEX, {
    -- https://github.com/markqvist/Reticulum/blob/192898864c008b6287dd56781d89fccef0bb5f7a/RNS/Packet.py#L72-L92
    [0x00] = "NONE",
    [0x01] = "RESOURCE",
    [0x02] = "RESOURCE_ADV",
    [0x03] = "RESOURCE_REQ",
    [0x04] = "RESOURCE_HMU",
    [0x05] = "RESOURCE_PRF",
    [0x06] = "RESOURCE_ICL",
    [0x07] = "RESOURCE_RCL",
    [0x08] = "CACHE_REQUEST",
    [0x09] = "REQUEST",
    [0x0A] = "RESPONSE",
    [0x0B] = "PATH_RESPONSE",
    [0x0C] = "COMMAND",
    [0x0D] = "COMMAND_STATUS",
    [0x0E] = "CHANNEL",
    [0xFA] = "KEEPALIVE",
    [0xFB] = "LINKIDENTIFY",
    [0xFC] = "LINKCLOSE",
    [0xFD] = "LINKPROOF",
    [0xFE] = "LRRTT",
    [0xFF] = "LRPROOF",
})
local f_payload = ProtoField.bytes("reticulum.payload", "Payload", base.NONE)

reticulum_proto.fields = {
    f_flags, f_flag_packet_type, f_flag_destination_type, f_flag_propagation_type,
    f_flag_context, f_flag_header_type, f_flag_ifac, f_hops, f_transport_dest, f_dest, f_context, f_payload
}

-- Source - https://stackoverflow.com/a/49709999
-- Posted by VasiliNovikov, modified by community. See post 'Timeline' for change history
-- Retrieved 2026-09-26, License - CC BY-SA 4.0
local function filter_inplace(arr, func)
    local new_index = 1
    local size_orig = #arr
    for old_index, v in ipairs(arr) do
        if func(v, old_index) then
            arr[new_index] = v
            new_index = new_index + 1
        end
    end
    for i = new_index, size_orig do arr[i] = nil end
end

local readable_fields = {
    "reticulum.flags.ptype",
    "reticulum.flags.proptype",
    "reticulum.dest",
}

for _, fname in pairs(readable_fields) do
    Field.new(fname)
end

local function get_field_info(fname)
    local finfos = { all_field_infos() }
    filter_inplace(finfos, function(finfo) return finfo.name == fname end)
    return finfos[#finfos]
end

local function get_field(fname)
    local finfo = get_field_info(fname)
    if finfo ~= nil then
        return finfo.value
    end
    return nil
end

function reticulum_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "RETICULUM"

    local subtree = tree:add(reticulum_proto, buffer(), "Reticulum Packet")
    local subtree_sub = nil

    local byte_ctr = 0

    -- very pretty flags
    local flags_buf = buffer(byte_ctr, 1)
    subtree_sub = subtree:add(f_flags, flags_buf)
    subtree_sub:add(f_flag_packet_type, flags_buf)
    subtree_sub:add(f_flag_destination_type, flags_buf)
    subtree_sub:add(f_flag_propagation_type, flags_buf)
    subtree_sub:add(f_flag_context, flags_buf)
    subtree_sub:add(f_flag_header_type, flags_buf)
    subtree_sub:add(f_flag_ifac, flags_buf)
    byte_ctr = byte_ctr + 1

    -- hop count
    subtree:add(f_hops, buffer(byte_ctr, 1))
    byte_ctr = byte_ctr + 1

    -- transport destination?
    if get_field("reticulum.flags.proptype") == 1 then
        subtree:add(f_transport_dest, buffer(byte_ctr, 16))
        byte_ctr = byte_ctr + 16
    end

    -- destination
    subtree:add(f_dest, buffer(byte_ctr, 16))
    byte_ctr = byte_ctr + 16

    -- context byte
    subtree:add(f_context, buffer(byte_ctr, 1))
    byte_ctr = byte_ctr + 1

    -- payload bytes
    subtree:add(f_payload, buffer(byte_ctr, buffer:len() - byte_ctr))

    local packet_info_text = nil

    local packet_type = get_field("reticulum.flags.ptype")
    -- Data
    if packet_type == 0 then
        packet_info_text = "Encrypted data to " .. "<" .. tostring(get_field("reticulum.dest")) .. ">"
    -- Announce
    elseif packet_type == 1 then
        packet_info_text = "Announce from " .. "<" .. tostring(get_field("reticulum.dest")) .. ">"
    end

    if packet_info_text ~= nil then
        local prev_info = tostring(pinfo.cols.info)
        pinfo.cols.info:set(prev_info .. (prev_info ~= "" and ", " or "") .. packet_info_text)
    end
end

reticulum_hdlc_table = DissectorTable.get("reticulum_hdlc")
reticulum_hdlc_table:add(0, reticulum_proto)
