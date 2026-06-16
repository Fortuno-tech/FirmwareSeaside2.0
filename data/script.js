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

document.getElementById("passwordModal").addEventListener("click", function (e) {
  if (e.target.id === "passwordModal") this.classList.remove("active");
});

// ================= RESEAUX =================
function changeNetworkMode() {
  let mode = document.getElementById("modeNet").value;
  let area = document.getElementById("networkArea");
  if (mode === "ap") {
    area.innerHTML = `<div><h3 style="margin:25px 0 20px 0;"><i class="bi bi-router"></i> Gestion Point d'accès</h3><div class="input-group"><label><i class="bi bi-wifi"></i> Nom Point d'accès</label><input type="text" id="apSSIDInput" placeholder="ESP32-NETWORK" minlength="4" maxlength="20" required></div><div class="input-group"><label><i class="bi bi-lock-fill"></i> Mot de passe</label><div class="password-group"><input type="password" id="apPassword" placeholder="********" minlength="8" maxlength="20" required><i class="bi bi-eye" onclick="togglePassword('apPassword', this)"></i></div></div><button class="btn" type="submit"><i class="bi bi-check-circle"></i> Confirmer</button></div>`;
    
    // Charger et pré-remplir les paramètres AP actuels
    fetch("/api/config/ap")
      .then(res => res.json())
      .then(data => {
        document.getElementById("apSSIDInput").value = data.ssid || "";
        document.getElementById("apPassword").value = data.password || "";
      })
      .catch(err => console.error("Erreur AP fetch :", err));
  } else if (mode === "online") {
    area.innerHTML = `<div><h3 style="margin:25px 0 20px 0;"><i class="bi bi-globe"></i> Mode En Ligne</h3><div class="input-group"><label><i class="bi bi-router"></i> SSID WiFi</label><input type="text" id="wifiSSIDInput" placeholder="Nom WiFi" minlength="4" maxlength="20" required></div><div class="input-group"><label><i class="bi bi-lock-fill"></i> Mot de passe</label><div class="password-group"><input type="password" id="wifiPassword" placeholder="********" minlength="8" maxlength="20" required><i class="bi bi-eye" onclick="togglePassword('wifiPassword', this)"></i></div></div>
    
    <h3 style="margin:25px 0 20px 0;"><i class="bi bi-broadcast"></i> Serveur MQTT Mosquitto</h3>
    <div class="input-group"><label><i class="bi bi-server"></i> Adresse IP du Broker</label><input type="text" id="mqttServerInput" placeholder="192.168.1.2" required></div>
    <div class="input-group"><label><i class="bi bi-signpost-split"></i> Port du Broker</label><input type="number" id="mqttPortInput" placeholder="1883" required></div>
    
    <button class="btn" type="submit"><i class="bi bi-check-circle"></i> Confirmer</button></div>`;
    
    // Charger et pré-remplir les paramètres WiFi STA et MQTT actuels
    Promise.all([
      fetch("/api/wifi").then(res => res.json()),
      fetch("/api/mqtt").then(res => res.json())
    ]).then(([wifiData, mqttData]) => {
      document.getElementById("wifiSSIDInput").value = wifiData.ssid || "";
      document.getElementById("wifiPassword").value = wifiData.password || "";
      document.getElementById("mqttServerInput").value = mqttData.server || "192.168.1.2";
      document.getElementById("mqttPortInput").value = mqttData.port || 1883;
    })
    .catch(err => console.error("Erreur WiFi/MQTT fetch :", err));
  }
}

function isValidMAC(mac) {
  return /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/.test(mac);
}

// ================= MODULE =================
document.getElementById("moduleForm").addEventListener("submit", function (e) {
  e.preventDefault();
  let mac1 = document.getElementById("mac1").value.trim();
  let mac2 = document.getElementById("mac2").value.trim();
  let type = document.getElementById("typeModule").value;
  let typeEntree = document.getElementById("typeEntree").value;

  if (!isValidMAC(mac1) || !isValidMAC(mac2)) {
    Swal.fire({ icon: "error", title: "Adresse MAC invalide", text: "Format requis: AA:BB:CC:DD:EE:FF" });
    return;
  }

  fetch("/api/config/module", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ type, macMaster: mac1, macSlave: mac2, typeE: typeEntree }),
  }).then(res => res.json()).then(result => {
    Swal.fire({ icon: "success", title: "Module configuré", text: "Succès" });
  }).catch(err => Swal.fire({ icon: "error", title: "Erreur", text: err.message }));
});

function loadModuleConfig() {
  fetch("/api/config/module")
    .then(res => res.json())
    .then(data => {
      if (data.role) {
        document.getElementById("typeModule").value = data.role.charAt(0).toUpperCase() + data.role.slice(1);
      }
      if (data.masterMAC) {
        document.getElementById("mac1").value = data.masterMAC;
      }
    })
    .catch(err => console.error("Erreur module config fetch :", err));
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
  })
  .then(res => res.json())
  .then(data => {
    console.log("Compteur mis à jour sur l'ESP32 :", val);
  })
  .catch(err => console.error("Erreur de mise à jour du compteur :", err));
}

// ================= RESEAUX SUBMIT =================
function submit_reseau(event) {
  event.preventDefault();
  let mode = document.getElementById("modeNet").value;
  if (mode === "ap") {
    let ssid = document.getElementById("apSSIDInput").value.trim();
    let password = document.getElementById("apPassword").value.trim();
    
    fetch("/api/config/ap", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password }),
    }).then(res => res.json()).then(result => {
      Swal.fire({ icon: "success", title: "Succès", text: "Configuration AP enregistrée" });
    }).catch(err => Swal.fire({ icon: "error", title: "Erreur", text: err.message }));
  } else if (mode === "online") {
    let ssid = document.getElementById("wifiSSIDInput").value.trim();
    let password = document.getElementById("wifiPassword").value.trim();
    let server = document.getElementById("mqttServerInput").value.trim();
    let port = parseInt(document.getElementById("mqttPortInput").value) || 1883;

    fetch("/api/wifi", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password })
    })
    .then(resWifi => {
      if (!resWifi.ok) throw new Error("Échec de la configuration WiFi");
      return fetch("/api/mqtt", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ server, port })
      });
    })
    .then(resMqtt => {
      if (!resMqtt.ok) throw new Error("Échec de la configuration MQTT");
      Swal.fire({ icon: "success", title: "Succès", text: "Configuration WiFi & MQTT enregistrée avec succès !" });
    })
    .catch(err => Swal.fire({ icon: "error", title: "Erreur", text: err.message }));
  }
}

// ================= FIRMWARE =================
function changeFirmwareMode() {
  let mode = document.getElementById("firmwareMode").value;
  let area = document.getElementById("firmwareArea");
  if (mode === "offline") {
    area.innerHTML = `<div><h3 style="margin:25px 0 20px 0;"><i class="bi bi-usb-drive"></i> Mise à jour Hors Ligne</h3><div class="input-group"><label><i class="bi bi-file-earmark-arrow-up"></i> Sélectionner Firmware</label><input type="file" id="firmwareFile" accept=".bin" required></div><button class="btn" type="button" onclick="uploadOfflineFirmware()"><i class="bi bi-upload"></i> Téléverser Firmware</button></div>`;
  } else {
    area.innerHTML = `<div><h3 style="margin:25px 0 20px 0;"><i class="bi bi-cloud-arrow-down"></i> Mise à jour En Ligne</h3><div class="input-group"><label><i class="bi bi-hdd-network"></i> Choisir le module</label><select id="onlineModule"><option>Module Master</option></select></div><div class="input-group"><label><i class="bi bi-link-45deg"></i> URL Firmware</label><input type="url" id="firmwareUrl" placeholder="https://example.com/firmware.bin" required></div><button class="btn" type="button" onclick="uploadOnlineFirmware()"><i class="bi bi-cloud-upload"></i> Télécharger et Téléverser</button></div>`;
  }
}

function uploadOfflineFirmware() {
  let file = document.getElementById("firmwareFile").files[0];
  if (!file) return;
  let formData = new FormData();
  formData.append("firmware", file);
  fetch("/update", { method: "POST", body: formData })
    .then(() => Swal.fire({ icon: "success", title: "Réussi", text: "Mise à jour lancée" }))
    .catch(err => Swal.fire({ icon: "error", title: "Erreur", text: err.message }));
}

// ================= DATA FETCHING =================
function updateDeviceStatus() {
  fetch('/api/status')
    .then(res => res.json())
    .then(data => {
      if (document.getElementById('passageCount')) document.getElementById('passageCount').innerText = data.total;
      if (document.getElementById('deviceMAC')) document.getElementById('deviceMAC').innerText = data.mac || "Inconnu";
      if (document.getElementById('deviceRole')) document.getElementById('deviceRole').innerText = (data.role || "neutral").toUpperCase();
      if (document.getElementById('deviceMode')) {
        document.getElementById('deviceMode').innerText = data.connected ? "En ligne (STA + AP)" : "Hors ligne (AP)";
      }
      
      // Mettre à jour la carte Mode du Dashboard
      let dashMode = document.getElementById('dashMode');
      if (dashMode) {
        dashMode.innerText = data.connected ? "En ligne" : "Hors ligne";
        let icon = dashMode.parentElement.nextElementSibling.querySelector('i');
        if (icon) {
          if (data.connected) {
            icon.className = "bi bi-wifi";
            icon.parentElement.className = "card-icon green";
          } else {
            icon.className = "bi bi-wifi-off";
            icon.parentElement.className = "card-icon red";
          }
        }
      }
    })
    .catch(err => console.error('Status Error:', err));
}

// ================= INIT =================
function showSettingPage(pageId, element) {
  document.querySelectorAll(".setting-page").forEach(p => p.classList.remove("active"));
  document.getElementById(pageId).classList.add("active");
  document.querySelectorAll(".settings-btn").forEach(b => b.classList.remove("active"));
  element.classList.add("active");
}

function togglePassword(inputId, icon) {
  let input = document.getElementById(inputId);
  if (input.type === "password") {
    input.type = "text";
    icon.classList.replace("bi-eye", "bi-eye-slash");
  } else {
    input.type = "password";
    icon.classList.replace("bi-eye-slash", "bi-eye");
  }
}

document.addEventListener("DOMContentLoaded", () => {
    changeNetworkMode();
    changeFirmwareMode();
    loadModuleConfig();
    setInterval(updateDeviceStatus, 3000);
    updateDeviceStatus();
});
