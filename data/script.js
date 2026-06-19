let titleAnimationInterval = null;
let titleAnimationTimeout = null;

function animatePageTitle(text) {
  const title = document.getElementById("pageTitle");

  // arrêter l'ancienne animation
  clearInterval(titleAnimationInterval);
  clearTimeout(titleAnimationTimeout);

  function play() {
    let index = 0;
    title.textContent = "";

    titleAnimationInterval = setInterval(() => {
      title.textContent += text.charAt(index);
      index++;

      if (index >= text.length) {
        clearInterval(titleAnimationInterval);

        // attendre puis recommencer
        titleAnimationTimeout = setTimeout(play, 1800);
      }
    }, 120); // vitesse d'apparition
  }

  play();
}

// ================= SIDEBAR =================
function toggleSidebar() {
  document.getElementById("sidebar").classList.toggle("active");
}

// fermer sidebar mobile
document.addEventListener("click", function (e) {
  let sidebar = document.getElementById("sidebar");
  let burger = document.querySelector(".burger");

  if (window.innerWidth <= 768) {
    if (!sidebar.contains(e.target) && !burger.contains(e.target)) {
      sidebar.classList.remove("active");
    }
  }
});

// ================= TOASTIFY FUNCTION =================
function showToast(message, type = "success") {
  const colors = {
    success: "#2563eb",
    error: "#dc2626",
    warning: "#ea580c",
    info: "#475569",
  };

  Toastify({
    text: message,
    duration: 3000,
    gravity: "top",
    position: "right",
    close: true,
    stopOnFocus: true,
    style: {
      background: colors[type],
      color: "#fff",
      borderRadius: "12px",
      boxShadow: "0 10px 25px rgba(0,0,0,.15)",
    },
  }).showToast();
}

// ================= NAVIGATION =================
function showPage(page, element) {
  // pages
  document
    .querySelectorAll(".page")
    .forEach((p) => p.classList.remove("active"));

  document.getElementById(page).classList.add("active");

  // menu active
  document
    .querySelectorAll(".menu a")
    .forEach((link) => link.classList.remove("active"));

  element.classList.add("active");

  // title
  let title = document.getElementById("pageTitle");

  let pageTitle = "";

  switch (page) {
    case "dashboard":
      pageTitle = "Dashboard";
      break;

    case "reseaux":
      pageTitle = "Réseaux";
      break;

    case "module":
      pageTitle = "Module";
      break;

    case "licence":
      pageTitle = "Licence";
      break;

    case "parametre":
      pageTitle = "Paramètre";
      break;
  }

  animatePageTitle(pageTitle);

  // mobile
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }
}

// ================= PASSWORD MODAL =================
let currentParamElement = null;

function openPasswordModal(element) {
  currentParamElement = element;

  // Fermer la sidebar sur mobile
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }

  document.getElementById("passwordModal").classList.add("active");
  document.getElementById("adminPassword").value = "";
  document.getElementById("passwordError").innerText = "";

  setTimeout(() => {
    document.getElementById("adminPassword").focus();
  }, 100);

  document
    .getElementById("adminPassword")
    .addEventListener("keydown", function (e) {
      if (e.key === "Enter") {
        confirmPassword();
      }
    });
}

function confirmPassword() {
  let password = document.getElementById("adminPassword").value;

  if (password === "1112") {
    document.getElementById("passwordModal").classList.remove("active");

    showToast("Accès autorisé", "success");

    setTimeout(() => {
      showPage("parametre", currentParamElement);
    }, 400);

    return;
  }

  document.getElementById("passwordError").innerText = "Mot de passe incorrect";
}

// fermer modal dehors
document
  .getElementById("passwordModal")
  .addEventListener("click", function (e) {
    if (e.target.id === "passwordModal") {
      this.classList.remove("active");
    }
  });

// ================= WEBSOCKET (AJOUTÉ DEPUIS script_farany) =================
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

// ================= MAC VALIDATION =================
function isValidMAC(mac) {
  let regex = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;
  return regex.test(mac);
}

// ================= MODULE =================
document.getElementById("moduleForm").addEventListener("submit", function (e) {
  e.preventDefault();

  let mac1 = document.getElementById("mac1").value.trim();
  let mac2 = document.getElementById("mac2").value.trim();
  let type = document.getElementById("typeModule").value;
  let typeEntree = document.getElementById("typeEntree").value;

  if (!isValidMAC(mac1)) {
    showToast("Adresse MAC invalide (AA:BB:CC:DD:EE:FF)", "error");
    return;
  }

  if (!isValidMAC(mac2)) {
    showToast("Veuillez vérifier l'adresse MAC du module associé", "error");
    return;
  }

  if (typeEntree === "") {
    showToast("Veuillez sélectionner un type d'entrée", "warning");
    return;
  }

  const data = {
    type: type,
    macMaster: mac1,
    macSlave: mac2,
    typeE: typeEntree,
  };

  fetch("/api/config/module", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(data),
  })
    .then((response) => response.json())
    .then((result) => {
      showToast(result.message || "Module configuré avec succès", "success");
    })
    .catch((error) => {
      showToast(error.message, "error");
    });

  let card = document.createElement("div");
  card.className = "module-card";
  card.innerHTML = `
  <h4>
    <i class="bi bi-hdd-network"></i>
    Module ${type}
  </h4>

  <p>
    <i class="bi bi-door-open"></i>
    Entrée : ${typeEntree}
  </p>

  <p>
    <i class="bi bi-activity"></i>
    Passage : 50
  </p>

  <p>
    <i class="bi bi-wifi"></i>
    Mode : En ligne
  </p>

  <p>
    <i class="bi bi-battery-full"></i>
    Batterie : 100%
  </p>
`;

  document.getElementById("moduleList").appendChild(card);
  this.reset();
});

// ================= LOAD MODULE CONFIG (AJOUTÉ DEPUIS script_farany) =================
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

// ================= PASSAGE (AMÉLIORÉ DEPUIS script_farany) =================
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

// ================= RESEAUX =================
function changeNetworkMode() {
  let mode = document.getElementById("modeNet").value;
  let area = document.getElementById("networkArea");

  // AP
  if (mode === "ap") {
    area.innerHTML = `
  <div>
    <h3 style="margin:25px 0 20px 0;">
      <i class="bi bi-router"></i> 
      Gestion Point d'accès
    </h3>
    <div class="input-group">
      <label>
      <i class="bi bi-wifi"></i> 
        Nom Point d'accès
      </label>
      <input type="text" id="apSSIDInput" placeholder="ESP32-NETWORK" minlength="4" maxlength="20" required>
    </div>
    <div class="input-group">
      <label>
        <i class="bi bi-lock-fill"></i>
        Mot de passe</label>
        <div class="password-group">
          <input type="password" id="apPassword" placeholder="********" minlength="8" maxlength="20" required>
            <i class="bi bi-eye" onclick="togglePassword('apPassword', this)"></i>
        </div>
    </div>
    <button class="btn" type="submit" onclick="submit_reseau(event)">
      <i class="bi bi-check-circle"></i>
      Confirmer
      </button>
    </div>`;

    // Charger et pré-remplir les paramètres AP actuels
    fetch("/api/config/ap")
      .then((res) => res.json())
      .then((data) => {
        document.getElementById("apSSIDInput").value = data.ssid || "";
        document.getElementById("apPassword").value = data.password || "";
      })
      .catch((err) => console.error("Erreur AP fetch :", err));
  }

  // ONLINE
  if (mode === "online") {
    area.innerHTML = `
    <div>
      <h3 style="margin:25px 0 20px 0;">
      <i class="bi bi-globe"></i>
      Mode En Ligne
      </h3>
      <div class="input-group">
        <label>
        <i class="bi bi-router"></i>
        SSID WiFi
        </label>
        <input type="text" id="wifiSSIDInput" placeholder="Nom WiFi" minlength="4" maxlength="20" required>
      </div>
      <div class="input-group">
        <label>
        <i class="bi bi-lock-fill"></i>
        Mot de passe
        </label>
        <div class="password-group">
          <input type="password" id="wifiPassword" placeholder="********" minlength="8" maxlength="20" required>
          <i class="bi bi-eye" onclick="togglePassword('wifiPassword', this)"></i>
        </div>
      </div>
    
      <h3 style="margin:25px 0 20px 0;">
        <i class="bi bi-broadcast"></i>
        Serveur MQTT Mosquitto
      </h3>
      <div class="input-group">
        <label>
        <i class="bi bi-server"></i>
        Adresse IP du Broker
        </label>
        <input type="text" id="mqttServerInput" placeholder="192.168.1.2" required>
      </div>
      <div class="input-group">
        <label>
        <i class="bi bi-signpost-split"></i>
        Port du Broker
        </label>
        <input type="number" id="mqttPortInput" placeholder="1883" required>
      </div>
      
      <button class="btn" type="submit" onclick="submit_reseau(event)">
      <i class="bi bi-check-circle"></i>
      Confirmer
      </button>
    </div>`;
    // Charger et pré-remplir les paramètres WiFi STA et MQTT actuels
    Promise.all([
      fetch("/api/wifi").then((res) => res.json()),
      fetch("/api/mqtt").then((res) => res.json()),
    ])
      .then(([wifiData, mqttData]) => {
        document.getElementById("wifiSSIDInput").value = wifiData.ssid || "";
        document.getElementById("wifiPassword").value = wifiData.password || "";
        document.getElementById("mqttServerInput").value =
          mqttData.server || "192.168.1.2";
        document.getElementById("mqttPortInput").value = mqttData.port || 1883;
      })
      .catch((err) => console.error("Erreur WiFi/MQTT fetch :", err));
  }
}

// ================= RESEAUX SUBMIT =================
function submit_reseau(event) {
  event.preventDefault();

  let mode = document.getElementById("modeNet").value;

  if (mode === "ap") {
    let ssid = document.getElementById("apSSIDInput").value.trim();
    let password = document.getElementById("apPassword").value.trim();

    if (ssid === "" || password === "") {
      showToast("Veuillez remplir tous les champs", "warning");
      return;
    }

    if (ssid.length < 4 || ssid.length > 20) {
      showToast("Le nom doit contenir entre 4 et 20 caractères", "warning");
      return;
    }

    if (password.length < 8 || password.length > 20) {
      showToast("Le mot de passe doit contenir entre 8 et 20 caractères", "warning");
      return;
    }

    const data = { ssid, password };

    fetch("/api/config/ap", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    })
      .then((response) => response.json())
      .then((result) => {
        showToast(result.message || "Configuration AP enregistrée", "success");
      })
      .catch((error) => {
        showToast(error.message, "error");
      });
  } else if (mode === "online") {
    let ssid = document.getElementById("wifiSSIDInput").value.trim();
    let password = document.getElementById("wifiPassword").value.trim();
    let server = document.getElementById("mqttServerInput").value.trim();
    let port = parseInt(document.getElementById("mqttPortInput").value) || 1883;

    if (ssid === "" || password === "" || server === "") {
      showToast("Veuillez remplir tous les champs", "warning");
      return;
    }

    if (ssid.length < 4 || ssid.length > 20) {
      showToast("Le nom WiFi doit contenir entre 4 et 20 caractères", "warning");
      return;
    }

    if (password.length < 8 || password.length > 20) {
      showToast("Le mot de passe WiFi doit contenir entre 8 et 20 caractères", "warning");
      return;
    }

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
        showToast("Configuration WiFi & MQTT enregistrée avec succès !", "success");
      })
      .catch(err => showToast(err.message, "error"));
  }
}

// ================= FIRMWARE =================
function changeFirmwareMode() {
  let mode = document.getElementById("firmwareMode").value;
  let area = document.getElementById("firmwareArea");

  // ================= HORS LIGNE =================
  if (mode === "offline") {
    area.innerHTML = `
      <div>
        <h3 style="margin:25px 0 20px 0;">
          <i class="bi bi-usb-drive"></i>
          Mise à jour Hors Ligne
        </h3>
        <div class="input-group">
          <label>
            <i class="bi bi-file-earmark-arrow-up"></i>
            Sélectionner Firmware
          </label>
          <input
            type="file"
            id="firmwareFile"
            accept=".bin"
            required
          >
        </div>
        <button
          class="btn"
          type="button"
          onclick="uploadOfflineFirmware()"
        >
          <i class="bi bi-upload"></i>
          Téléverser Firmware
        </button>
      </div>
    `;
  }

  // ================= EN LIGNE =================
  if (mode === "online") {
    area.innerHTML = `
      <div>
        <h3 style="margin:25px 0 20px 0;">
          <i class="bi bi-cloud-arrow-down"></i>
          Mise à jour En Ligne
        </h3>
        <div class="input-group">
          <label>
            <i class="bi bi-hdd-network"></i>
            Choisir le module
          </label>
          <select id="onlineModule">
            <option>Module Master</option>
            <option>Module Slave 1</option>
            <option>Module Slave 2</option>
            <option>Module Slave 3</option>
          </select>
        </div>
        <div class="input-group">
          <label>
            <i class="bi bi-link-45deg"></i>
            URL Firmware
          </label>
          <input
            type="url"
            id="firmwareUrl"
            placeholder="https://example.com/firmware.bin"
            required
          >
        </div>
        <button
          class="btn"
          type="button"
          onclick="uploadOnlineFirmware()"
        >
          <i class="bi bi-cloud-upload"></i>
          Télécharger et Téléverser
        </button>
      </div>
    `;
  }
}

// ================= UPLOAD OFFLINE =================
function uploadOfflineFirmware() {
  let file = document.getElementById("firmwareFile").files[0];

  if (!file) {
    showToast("Veuillez sélectionner un fichier .bin", "warning");
    return;
  }

  // vérification extension
  if (!file.name.endsWith(".bin")) {
    showToast("Seuls les fichiers .bin sont acceptés", "error");
    return;
  }

  let formData = new FormData();
  formData.append("firmware", file);

  fetch("/update", {
    method: "POST",
    body: formData,
  })
    .then((response) => response.json())
    .then((result) => {
      showToast(result.message || "Firmware envoyé avec succès", "success");
    })
    .catch((error) => {
      showToast(error.message, "error");
    });
}

// ================= UPLOAD ONLINE =================
function uploadOnlineFirmware() {
  let module = document.getElementById("onlineModule").value;
  let url = document.getElementById("firmwareUrl").value.trim();

  if (url === "") {
    showToast("Veuillez saisir l'adresse du firmware", "warning");
    return;
  }

  // simple validation
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    showToast("L'adresse doit commencer par http:// ou https://", "error");
    return;
  }

  const data = {
    module: module,
    firmwareUrl: url,
  };

  fetch("http://localhost:3000/update-online", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(data),
  })
    .then((response) => response.json())
    .then((result) => {
      showToast(result.message || "Le téléchargement du firmware a démarré", "success");
    })
    .catch((error) => {
      showToast(error.message, "error");
    });
}

// ================= PARAMETRE SUB MENU =================
function showSettingPage(pageId, element) {
  // cacher pages
  document.querySelectorAll(".setting-page").forEach((page) => {
    page.classList.remove("active");
  });

  // afficher page
  document.getElementById(pageId).classList.add("active");

  // bouton active
  document.querySelectorAll(".settings-btn").forEach((btn) => {
    btn.classList.remove("active");
  });

  element.classList.add("active");
}

// ================= TOGGLE PASSWORD =================
function togglePassword(inputId, icon) {
  let input = document.getElementById(inputId);

  if (input.type === "password") {
    input.type = "text";
    icon.classList.remove("bi-eye");
    icon.classList.add("bi-eye-slash");
  } else {
    input.type = "password";
    icon.classList.remove("bi-eye-slash");
    icon.classList.add("bi-eye");
  }
}

// ================= LICENCE =================
function randomPart(length) {
  const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const random = new Uint32Array(length);
  crypto.getRandomValues(random);
  let text = "";
  for (let i = 0; i < length; i++) {
    text += chars[random[i] % chars.length];
  }
  return text;
}

function generateLicense() {
  return `SMC-${new Date().getFullYear()}-${randomPart(4)}-${randomPart(4)}-${randomPart(4)}-${randomPart(4)}`;
}

function generateNewLicense() {
  const licence = generateLicense();
  document.getElementById("licenseCode").value = licence;
  document.getElementById("licenseDate").value = new Date().toLocaleString();
}

function copyLicense() {
  const licence = document.getElementById("licenseCode").value;

  if (licence === "") {
    showToast("Aucune licence générée", "warning");
    return;
  }

  navigator.clipboard.writeText(licence);
  showToast("Licence copiée", "success");
}

function sendLicense() {
  const licence = document.getElementById("licenseCode").value;

  if (licence === "") {
    showToast("Générez une licence d'abord", "warning");
    return;
  }

  fetch("http://localhost:3000/licence", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      licence: licence,
      date: new Date().toISOString(),
    }),
  })
    .then((r) => r.json())
    .then((result) => {
      showToast(result.message || "Licence envoyée", "success");
    })
    .catch((error) => {
      showToast(error.message, "error");
    });
}

// ================= SENSOR THRESHOLD =================
function loadSensorConfig() {
  fetch('/api/config/sensor')
    .then(res => res.json())
    .then(data => {
      const input  = document.getElementById('seuilInput');
      const slider = document.getElementById('seuilSlider');
      const label  = document.getElementById('seuilValue');
      if (!input) return;
      if (data.seuil !== undefined) {
        input.value  = data.seuil;
        if (slider) {
          slider.min   = data.seuilMin || 10;
          slider.max   = data.seuilMax || 500;
          slider.value = data.seuil;
        }
        if (label) label.textContent = data.seuil + ' cm';
      }
    })
    .catch(err => console.error('Erreur fetch seuil :', err));
}

function syncSeuilSlider(val) {
  const input = document.getElementById('seuilInput');
  const label = document.getElementById('seuilValue');
  if (input) input.value = val;
  if (label) label.textContent = val + ' cm';
}

function syncSeuilInput(val) {
  const slider = document.getElementById('seuilSlider');
  const label  = document.getElementById('seuilValue');
  if (slider) slider.value = val;
  if (label)  label.textContent = val + ' cm';
}

function saveSensorConfig() {
  const input = document.getElementById('seuilInput');
  if (!input) return;
  const val = parseInt(input.value);
  if (isNaN(val) || val < 10 || val > 500) {
    showToast('Valeur invalide (10 – 500 cm)', 'error');
    return;
  }
  fetch('/api/config/sensor', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ seuil: val })
  })
    .then(res => res.json())
    .then(data => {
      if (data.status === 'ok') {
        showToast('Seuil mis à jour : ' + val + ' cm', 'success');
      } else {
        showToast(data.error || 'Erreur serveur', 'error');
      }
    })
    .catch(err => {
      console.error('Erreur save seuil :', err);
      showToast('Erreur de connexion', 'error');
    });
}

// ================= DATA FETCHING (AJOUTÉ DEPUIS script_farany) =================
function updateDeviceStatus() {
  fetch('/api/status')
    .then(res => res.json())
    .then(data => {
      // Si WebSocket n'est pas ouvert, on utilise les données du fetch
      if ((!websocket || websocket.readyState !== WebSocket.OPEN) && document.getElementById('passageCount')) {
        document.getElementById('passageCount').innerText = data.total || 0;
      }
      
      if (document.getElementById('deviceMAC')) 
        document.getElementById('deviceMAC').innerText = data.mac || "Inconnu";
      if (document.getElementById('deviceRole')) 
        document.getElementById('deviceRole').innerText = (data.role || "neutral").toUpperCase();
      if (document.getElementById('deviceMode')) {
        document.getElementById('deviceMode').innerText = data.connected ? "En ligne (STA + AP)" : "Hors ligne (AP)";
      }

      let dashMode = document.getElementById('dashMode');
      if (dashMode) {
        dashMode.innerText = data.connected ? "En ligne" : "Hors ligne";
        let icon = dashMode.parentElement.nextElementSibling?.querySelector('i');
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
document.addEventListener("DOMContentLoaded", () => {
  // Initialisation WebSocket (depuis script_farany)
  initWebSocket();
  
  // Chargement de la configuration du module (depuis script_farany)
  loadModuleConfig();
  
  // Initialisation des modes réseau et firmware
  changeNetworkMode();
  changeFirmwareMode();
  
  // Animation du titre
  animatePageTitle("Dashboard");
  
  // Chargement du seuil de détection
  loadSensorConfig();

  // Mise à jour périodique du statut (depuis script_farany)
  setInterval(updateDeviceStatus, 5000);
  updateDeviceStatus();
});