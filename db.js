// db.js
// Realtime Database (RTDB) data layer.
// Reads the paths written by the ESP32 code you showed:
//   /waterbuddy/latest
//   /waterbuddy/logs/<timestamp>
//
// firebase-config.js must run first and set window.rtdb.

import {
    ref,
    onValue,
    onChildAdded,
    query,
    orderByKey,
    limitToLast,
    off,
  } from "https://www.gstatic.com/firebasejs/12.9.0/firebase-database.js";
  
  function requireRTDB() {
    const rtdb = window.rtdb;
    if (!rtdb) {
      throw new Error(
        "Realtime Database not initialized (window.rtdb missing). Did firebase-config.js load?"
      );
    }
    return rtdb;
  }
  
  // Convert user input like "waterbuddy" or "/waterbuddy" to "waterbuddy"
  export function normalizeRootPath(rootPath) {
    const p = String(rootPath ?? "").trim();
    const noSlashes = p.replace(/^\/+|\/+$/g, "");
    return noSlashes || "waterbuddy";
  }
  
  export function latestPath(rootPath = "waterbuddy") {
    const root = normalizeRootPath(rootPath);
    return `${root}/latest`;
  }
  
  export function logsPath(rootPath = "waterbuddy") {
    const root = normalizeRootPath(rootPath);
    return `${root}/logs`;
  }
  
  /**
   * Subscribe to /<root>/latest and call `callback(value)` on every change.
   * Returns an unsubscribe function.
   */
  export function subscribeLatest(rootPath, callback) {
    const rtdb = requireRTDB();
    const r = ref(rtdb, latestPath(rootPath));
  
    const handler = (snapshot) => {
      callback(snapshot.exists() ? snapshot.val() : null);
    };
  
    onValue(r, handler, (err) => {
      console.error("RTDB latest subscription error", err);
      callback(null, err);
    });
  
    return () => off(r, "value", handler);
  }
  
  /**
   * Subscribe to the last N children in /<root>/logs.
   * Calls `onLog({ key, value })` for each existing and new log.
   * Returns an unsubscribe function.
   */
  export function subscribeRecentLogs(rootPath, n, onLog) {
    const rtdb = requireRTDB();
    const base = ref(rtdb, logsPath(rootPath));
    const q = query(base, orderByKey(), limitToLast(Math.max(1, Number(n) || 10)));
  
    const handler = (snapshot) => {
      onLog({ key: snapshot.key, value: snapshot.val() });
    };
  
    onChildAdded(q, handler, (err) => {
      console.error("RTDB logs subscription error", err);
    });
  
    return () => off(base, "child_added", handler);
  }