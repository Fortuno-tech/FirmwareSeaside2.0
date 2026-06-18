// ================= SIDEBAR =================
function toggleSidebar() {
  document.getElementById("sidebar").classList.toggle("active");
}

document.addEventListener("click", function (e) {
  let sidebar = document.getElementById("sidebar");
  let burger = document.querySelector(".burger");
  if (window.innerWidth <= 768) {
    if (!sidebar.contains(e.target) && !burger.contains(e.target)) {
      sidebar.classList.remove("active");
    }
  }
});

// ================= NAVIGATION =================
function showPage(page, element) {
  document.querySelectorAll(".page").forEach((p) => p.classList.remove("active"));
  document.getElementById(page).classList.add("active");
  document.querySelectorAll(".menu a").forEach((link) => link.classList.remove("active"));
  element.classList.add("active");

  let title = document.getElementById("pageTitle");
  if (page === "dashboard") title.innerText = "Dashboard";
  if (page === "reseaux") title.innerText = "Réseaux";
  if (page === "module") title.innerText = "Module";
  if (page === "parametre") title.innerText = "Paramètre";

  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }
}

// ================= PASSWORD MODAL =================
let currentParamElement = null;
function openPasswordModal(element) {
  currentParamElement = element;
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }
  document.getElementById("passwordModal").classList.add("active");
  document.getElementById("adminPassword").value = "";
  document.getElementById("passwordError").innerText = "";
  setTimeout(() => { document.getElementById("adminPassword").focus(); }, 100);
}

function confirmPassword() {
  let password = document.getElementById("adminPassword").value;
  if (password === "1112") {
    document.getElementById("passwordModal").classList.remove("active");
    Swal.fire({
      icon: "success",
      title: "Accès autorisé",
      text: "Bienvenue dans les paramètres",
      confirmButtonText: "Continuer",
      confirmButtonColor: "#2563eb",
    }).then(() => { showPage("parametre", currentParamElement); });
    return;
  }
  document.getElementById("passwordError").innerText = "Mot de passe incorrect";
}

// ================= WEBSOCKET =================
var gateway = "ws://" + window.location.hostname + "/ws";
var websocket;

function initWebSocket() {
    console.log("Tentative de connexion WebSocket...");
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log("Connexion WebSocket établie");
}

function onClose(event) {
    console.log("Connexion WebSocket fermée");
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log("Message WebSocket reçu:", event.data);
    if (document.getElementById('passageCount')) {
        document.getElementById('passageCount').innerText = event.data;
    }
}

// ================= PASSAGE =================
function incrementPassage() {
  let currentVal = parseInt(document.getElementById("passageCount").innerText) || 0;
  let newVal = currentVal + 1;
  updateCountOnServer(newVal);
}

function decrementPassage() {
  let currentVal = parseInt(document.getElementById("passageCount").innerText) || 0;
  if (currentVal > 0) {
    let newVal = currentVal - 1;
    updateCountOnServer(newVal);
  }
}

function updateCountOnServer(val) {
  document.getElementById("passageCount").innerText = val;
  fetch("/api/count", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ count: val })
  });
}

// ================= DATA FETCHING =================
function updateDeviceStatus() {
  fetch('/api/status')
    .then(res => res.json())
    .then(data => {
      if (websocket && websocket.readyState !== WebSocket.OPEN && document.getElementById('passageCount')) {
          document.getElementById('passageCount').innerText = data.total;
      }
      
      if (document.getElementById('deviceMAC')) document.getElementById('deviceMAC').innerText = data.mac || "Inconnu";
      if (document.getElementById('deviceRole')) document.getElementById('deviceRole').innerText = (data.role || "neutral").toUpperCase();
      if (document.getElementById('deviceMode')) {
        document.getElementById('deviceMode').innerText = data.connected ? "En ligne (STA + AP)" : "Hors ligne (AP)";
      }

      let dashMode = document.getElementById('dashMode');
      if (dashMode) {
        dashMode.innerText = data.connected ? "En ligne" : "Hors ligne";
      }
    })
    .catch(err => console.error('Status Error:', err));
}

// ================= INIT =================
document.addEventListener("DOMContentLoaded", () => {
    initWebSocket();
    loadModuleConfig();
    setInterval(updateDeviceStatus, 5000);
    updateDeviceStatus();
});

function loadModuleConfig() {
  fetch("/api/config/module").then(res => res.json()).then(data => {
      if (data.role) document.getElementById("typeModule").value = data.role.charAt(0).toUpperCase() + data.role.slice(1);   
      if (data.masterMAC) document.getElementById("mac1").value = data.masterMAC;
  });
}

function togglePassword(inputId, icon) {
  let input = document.getElementById(inputId);
  input.type = input.type === "password" ? "text" : "password";
  icon.classList.toggle("bi-eye-slash");
}
