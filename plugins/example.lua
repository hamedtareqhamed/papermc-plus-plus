-- Sample PaperMC++ Lua Plugin
log_info("Initializing PaperMC++ Example Lua Plugin...")

function onPlayerJoin(username, uuid)
    log_info("Player joined server: " .. username .. " (UUID: " .. uuid .. ")")
end

function onBlockBreak(username, x, y, z)
    log_info("Player " .. username .. " broke block at coordinates (" .. x .. ", " .. y .. ", " .. z .. ")")
end

function onChatMessage(username, message)
    log_info("<" .. username .. "> " .. message)
end
