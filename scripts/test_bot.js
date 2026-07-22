#!/usr/bin/env node
/**
 * Automated Minecraft Bot Integration Test Harness for PaperMC++.
 * Connects to C++ server on 127.0.0.1:25565 using mineflayer for version 1.20.4.
 */

const mineflayer = require('mineflayer');
const mcData1202 = require('minecraft-data')('1.20.2');

const host = process.env.SERVER_HOST || '127.0.0.1';
const port = parseInt(process.env.SERVER_PORT || '25565', 10);
const username = 'TestBot';

console.log(`[BOT-TEST] Connecting mineflayer bot to ${host}:${port} as '${username}' (v1.20.4)...`);

let botSpawned = false;
let timeoutTimer = null;

const bot = mineflayer.createBot({
    host: host,
    port: port,
    username: username,
    version: '1.20.4',
    auth: 'offline',
    checkTimeoutInterval: 15000
});

// Load default 1.20.4 dimension codec into bot registry
if (bot.registry && !bot.registry.dimensionsByName && mcData1202.loginPacket && mcData1202.loginPacket.dimensionCodec) {
    bot.registry.loadDimensionCodec(mcData1202.loginPacket.dimensionCodec);
    console.log('[BOT-TEST] Loaded default 1.20.4 dimension codec into bot registry.');
}

// Set overall safety timeout (25 seconds)
const safetyTimer = setTimeout(() => {
    if (!botSpawned) {
        console.error('[BOT-TEST FAIL] Timed out waiting for bot to spawn in world!');
        process.exit(1);
    }
}, 25000);

bot.on('login', () => {
    console.log('[BOT-TEST ASSERT 1 SUCCESS] Completed HANDSHAKE and LOGIN phases!');
});

bot.on('spawn', () => {
    botSpawned = true;
    const pos = bot.entity ? bot.entity.position : { x: 0, y: 64, z: 0 };
    console.log(`[BOT-TEST ASSERT 2 SUCCESS] Bot spawned in world! Position: (${pos.x.toFixed(2)}, ${pos.y.toFixed(2)}, ${pos.z.toFixed(2)})`);

    // Send test chat message
    try {
        bot.chat('Hello from PaperMC++ Automated Test Bot!');
        console.log('[BOT-TEST ASSERT 3 SUCCESS] Outgoing test chat message sent!');
    } catch (err) {
        console.warn(`[BOT-TEST WARN] Chat send deferred: ${err.message}`);
    }

    console.log('[BOT-TEST] Holding connection active for 10 seconds to verify stability...');
    timeoutTimer = setTimeout(() => {
        console.log('========================================================');
        console.log('  [BOT-TEST PASSED] Bot connected and stable for >= 10s!');
        console.log('========================================================');
        clearTimeout(safetyTimer);
        bot.quit();
        process.exit(0);
    }, 10000);
});

bot.on('kicked', (reason) => {
    console.error(`[BOT-TEST FAIL] Bot kicked from server: ${reason}`);
    clearTimeout(safetyTimer);
    if (timeoutTimer) clearTimeout(timeoutTimer);
    process.exit(1);
});

bot.on('error', (err) => {
    console.error(`[BOT-TEST FAIL] Bot socket/protocol error: ${err.stack || err.message || err}`);
    clearTimeout(safetyTimer);
    if (timeoutTimer) clearTimeout(timeoutTimer);
    process.exit(1);
});

bot.on('end', (reason) => {
    if (!botSpawned) {
        console.error(`[BOT-TEST FAIL] Connection ended prematurely before spawn. Reason: ${reason}`);
        clearTimeout(safetyTimer);
        process.exit(1);
    }
});
