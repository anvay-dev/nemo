// firebase-config.js
// Initializes Firebase + Realtime Database (RTDB)
import { initializeApp } from "https://www.gstatic.com/firebasejs/12.9.0/firebase-app.js";
import { getDatabase } from "https://www.gstatic.com/firebasejs/12.9.0/firebase-database.js";

// TODO: Paste your Firebase config object here (from Firebase Console → Project settings → Your apps)
const firebaseConfig = {
  apiKey: "AIzaSyCaR0xJBnkqFh3KBto9Aaf0ylntUeax7Rs",
  authDomain: "nemooo.firebaseapp.com",
  // REQUIRED for Realtime Database
  databaseURL: "https://nemooo-a14ef-default-rtdb.firebaseio.com/",
  projectId: "nemooo",
  storageBucket: "nemooo.firebasestorage.app",
  messagingSenderId: "974633909925",
  appId: "1:974633909925:web:6f91fd9bd283c7056a974c"
};

// Initialize Firebase + Realtime Database
const app = initializeApp(firebaseConfig);
export const rtdb = getDatabase(app, firebaseConfig.databaseURL);

// Make available to the rest of your app (no imports needed yet)
window.firebaseApp = app;
window.rtdb = rtdb;
