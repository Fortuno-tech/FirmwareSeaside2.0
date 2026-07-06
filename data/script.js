let titleAnimationInterval = null;
let titleAnimationTimeout = null;

function animatePageTitle(text) {
  const title = document.getElementById("pageTitle");
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
        titleAnimationTimeout = setTimeout(play, 1800);
      }
    }, 120);
  }
  play();
}

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

// ================= TOASTIFY =================
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
  document.querySelectorAll(".page").forEach((p) => p.classList.remove("active"));
  document.getElementById(page).classList.add("active");
  document.querySelectorAll(".menu a").forEach((link) => link.classList.remove("active"));
  element.classList.add("active");

  const titles = {
    dashboard: "Dashboard",
    reseaux: "Réseaux",
    module: "Module",
    licence: "Licence",
    parametre: "Paramètre",
  };
  animatePageTitle(titles[page] || page);

  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }

  // Actions spécifiques à la page
  if (page === "module") {
    loadModuleConfig();
    loadConfiguredSlaves();
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
  setTimeout(() => document.getElementById("adminPassword").focus(), 100);

  document.getElementById("adminPassword").addEventListener("keydown", function (e) {
    if (e.key === "Enter") confirmPassword();
  });
}

function confirmPassword() {
  let password = document.getElementById("adminPassword").value;
  if (password === "1112") {
    document.getElementById("passwordModal").classList.remove("active");
    showToast("Accès autorisé", "success");
    setTimeout(() => showPage("parametre", currentParamElement), 400);
    return;
  }
  document.getElementById("passwordError").innerText = "Mot de passe incorrect";
}

document.getElementById("passwordModal").addEventListener("click", function (e) {
  if (e.target.id === "passwordModal") this.classList.remove("active");
});

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
  const el = document.getElementById("passageCount");
  if (el) {
    el.innerText = event.data;
    // Animer la valeur
    el.classList.remove("count-flash");
    void el.offsetWidth;
    el.classList.add("count-flash");
  }
}

// ================= MAC VALIDATION =================
function isValidMAC(mac) {
  return /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/.test(mac);
}

// ================= MODULE ROLE FIELDS TOGGLE =================
// ================= MODULE FIELD TOGGLES =================
function toggleLinkToMasterFields() {
  const isChecked = document.getElementById("linkToMasterCheckbox").checked;
  const section = document.getElementById("slaveWifiSection");
  if (section) {
    section.style.display = isChecked ? "block" : "none";
    document.getElementById("mac2").required = isChecked;
    document.getElementById("slaveMasterSSID").required = isChecked;
    document.getElementById("slaveMasterPassword").required = isChecked;
  }
}

// ================= MODULE FORM SUBMIT =================
document.getElementById("moduleForm").addEventListener("submit", function (e) {
  e.preventDefault();

  let mac1       = document.getElementById("mac1").value.trim();
  let modId      = document.getElementById("moduleIdInput").value.trim();
  let typeEntree = document.getElementById("typeEntree").value;
  let isSlave    = document.getElementById("linkToMasterCheckbox").checked;

  // Si on est déjà esclave (bouton Délier affiché)
  const isAlreadySlave = document.getElementById("unlinkMasterBtn").style.display !== "none";

  if (!isValidMAC(mac1)) {
    showToast("Adresse MAC locale invalide (AA:BB:CC:DD:EE:FF)", "error");
    return;
  }

  if (typeEntree === "") {
    showToast("Veuillez sélectionner un type d'entrée", "warning");
    return;
  }

  const data = {
    moduleId: modId || (isSlave || isAlreadySlave ? "Slave" : "Master"),
    macMaster: mac1,
    typeE: typeEntree,
  };

  if (isSlave || isAlreadySlave) {
    let mac2 = document.getElementById("mac2").value.trim();
    if (!isValidMAC(mac2)) {
      showToast("Adresse MAC du Master invalide", "error");
      return;
    }
    data.macSlave = mac2; // Le firmware associe macSlave au masterMAC dans le stockage
    data.wifiSSID = document.getElementById("slaveMasterSSID").value.trim();
    data.wifiPassword = document.getElementById("slaveMasterPassword").value.trim();

    if (data.wifiSSID === "") {
      showToast("Veuillez entrer le SSID WiFi du Master", "warning");
      return;
    }
    if (data.wifiPassword.length < 8) {
      showToast("Le mot de passe WiFi du Master doit faire au moins 8 caractères", "warning");
      return;
    }
  } else {
    // Si Master, on met les valeurs par défaut pour délier
    data.macSlave = "00:00:00:00:00:00";
    data.wifiSSID = "";
    data.wifiPassword = "";
  }

  showToast("Envoi de la configuration...", "info");

  fetch("/api/config/module", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  })
    .then((response) => {
      if (!response.ok) throw new Error("HTTP error " + response.status);
      return response.json();
    })
    .then(() => showToast("Configuration enregistrée. Le module redémarre...", "success"))
    .catch((error) => {
      if (isSlave || isAlreadySlave || error.message.includes("Failed to fetch") || error.message.includes("NetworkError")) {
        showToast("Configuration envoyée. Le module redémarre...", "success");
      } else {
        showToast("Erreur lors de la configuration: " + error.message, "error");
      }
    });
});

// ================= UNLINK FROM MASTER =================
function unlinkFromMaster() {
  if (!confirm("Délier ce module du Master et le faire repasser en Master autonome ? Le module va redémarrer.")) return;

  const data = {
    moduleId: "Master",
    macSlave: "00:00:00:00:00:00",
    wifiSSID: "",
    wifiPassword: "",
    typeE: document.getElementById("typeEntree").value || "vip"
  };

  showToast("Envoi de la commande de déliaison...", "info");

  fetch("/api/config/module", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  })
    .then(response => {
      showToast("Module délié. Redémarrage...", "success");
    })
    .catch(() => {
      showToast("Module délié. Redémarrage...", "success");
    });
}

// ================= LOAD MODULE CONFIG =================
function loadModuleConfig() {
  fetch("/api/config/module")
    .then(res => res.json())
    .then(data => {
      const isSlave = data.role && data.role.toLowerCase() === "slave";

      if (data.moduleId) document.getElementById("moduleIdInput").value = data.moduleId;
      if (data.masterMAC) document.getElementById("mac2").value = data.masterMAC;
      if (data.wifiSSID) document.getElementById("slaveMasterSSID").value = data.wifiSSID;
      if (data.wifiPassword) document.getElementById("slaveMasterPassword").value = data.wifiPassword;

      const toggleGroup = document.getElementById("linkToMasterToggleGroup");
      const unlinkBtn   = document.getElementById("unlinkMasterBtn");
      const wifiSection = document.getElementById("slaveWifiSection");
      const slavesConfigSection = document.getElementById("slavesConfigSection");

      if (isSlave) {
        // Mode Slave actif : désactiver les champs, masquer le toggle, afficher le bouton "Délier"
        if (toggleGroup) toggleGroup.style.display = "none";
        if (unlinkBtn) unlinkBtn.style.display = "block";
        if (wifiSection) {
          wifiSection.style.display = "block";
          document.getElementById("mac2").disabled = true;
          document.getElementById("slaveMasterSSID").disabled = true;
          document.getElementById("slaveMasterPassword").disabled = true;
        }
        if (slavesConfigSection) slavesConfigSection.style.display = "none";
      } else {
        // Mode Master : laisser cocher, activer les champs
        if (toggleGroup) toggleGroup.style.display = "block";
        if (unlinkBtn) unlinkBtn.style.display = "none";
        document.getElementById("linkToMasterCheckbox").checked = false;
        if (wifiSection) {
          wifiSection.style.display = "none";
          document.getElementById("mac2").disabled = false;
          document.getElementById("slaveMasterSSID").disabled = false;
          document.getElementById("slaveMasterPassword").disabled = false;
        }
        if (slavesConfigSection) slavesConfigSection.style.display = "block";
      }

      // Mettre à jour la bannière de rôle
      const roleBannerText = document.getElementById("roleBannerText");
      const roleBanner     = document.getElementById("roleBanner");
      if (roleBannerText && data.role) {
        const roleLabels = { master: "🟢 Ce module est le MASTER", slave: "🔵 Ce module est un SLAVE" };
        roleBannerText.textContent = (data.moduleId ? `${data.moduleId} — ` : "") + (roleLabels[data.role.toLowerCase()] || data.role.toUpperCase());
        if (roleBanner) roleBanner.className = "role-banner role-" + data.role.toLowerCase();
      }
    })
    .catch(err => console.error("Erreur module config fetch :", err));

  // Remplir automatiquement mac1
  fetch("/api/status")
    .then(res => res.json())
    .then(data => {
      let mac1Input = document.getElementById("mac1");
      if (mac1Input) {
        mac1Input.value    = data.mac || "";
        mac1Input.readOnly = true;
        mac1Input.style.backgroundColor = "rgba(255, 255, 255, 0.05)";
        mac1Input.style.cursor = "not-allowed";
      }
    })
    .catch(err => console.error("Erreur status fetch :", err));
}

// ================= LOAD CONFIGURED SLAVES =================
function loadConfiguredSlaves() {
  const container = document.getElementById("moduleList");
  if (!container) return;
  container.innerHTML = "";

  // Récupérer les slaves configurés + les slaves auto-découverts
  Promise.all([
    fetch("/api/slaves/configured").then(r => r.json()).catch(() => []),
    fetch("/api/slaves").then(r => r.json()).catch(() => []),
  ]).then(([configured, discovered]) => {
    // Fusionner : slaves configurés d'abord, puis auto-découverts non encore configurés
    const allMACs = new Set(configured.map(s => s.mac.toUpperCase()));
    const extra = discovered.filter(s => !allMACs.has(s.mac.toUpperCase()));
    const allSlaves = [...configured, ...extra];

    if (allSlaves.length === 0) {
      container.innerHTML = "<p class='empty-slaves'>Aucun module esclave. Cliquez sur <strong>Ajouter Slave</strong> pour en configurer un.</p>";
      return;
    }

    allSlaves.forEach(slave => {
      const isDiscovered  = !allMACs.has(slave.mac.toUpperCase());
      const card = document.createElement("div");
      card.className = "module-card";
      card.innerHTML = `
        <div class="module-card-header">
          <div class="module-card-icon">
            <i class="bi bi-cpu-fill"></i>
          </div>
          <div>
            <h4>${slave.moduleId || "Slave"}</h4>
            <span class="module-card-tag ${isDiscovered ? "tag-auto" : "tag-manual"}">
              ${isDiscovered ? "Auto-découvert" : "Configuré"}
            </span>
          </div>
          <button class="btn-remove-slave" onclick="deleteSlave('${slave.mac}')" title="Supprimer">
            <i class="bi bi-x-lg"></i>
          </button>
        </div>
        <div class="module-card-info">
          <p><i class="bi bi-cpu"></i> MAC : <code>${slave.mac}</code></p>
          ${slave.wifiSSID ? `<p><i class="bi bi-wifi"></i> SSID : ${slave.wifiSSID}</p>` : ""}
          ${slave.ip ? `<p><i class="bi bi-link-45deg"></i> IP : <a href="http://${slave.ip}" target="_blank">${slave.ip}</a></p>` : ""}
        </div>
      `;
      container.appendChild(card);
    });
  });
}

// ================= ADD SLAVE MODAL =================
function openAddSlaveModal() {
  document.getElementById("newSlaveId").value = "";
  document.getElementById("newSlaveMAC").value = "";
  document.getElementById("newSlaveSSID").value = "";
  document.getElementById("newSlavePassword").value = "";
  
  // Pré-remplir les informations WiFi du Master AP
  fetch("/api/config/ap")
    .then(res => res.json())
    .then(data => {
      if (data.ssid)     document.getElementById("newSlaveSSID").value = data.ssid;
      if (data.password) document.getElementById("newSlavePassword").value = data.password;
    })
    .catch(err => console.error("Erreur récupération config AP master :", err));

  document.getElementById("addSlaveModal").classList.add("active");
}

function closeAddSlaveModal() {
  document.getElementById("addSlaveModal").classList.remove("active");
}

document.getElementById("addSlaveModal").addEventListener("click", function (e) {
  if (e.target.id === "addSlaveModal") closeAddSlaveModal();
});

function confirmAddSlave() {
  const slaveId  = document.getElementById("newSlaveId").value.trim();
  const slaveMac = document.getElementById("newSlaveMAC").value.trim();
  const slaveSSID = document.getElementById("newSlaveSSID").value.trim();
  const slavePwd  = document.getElementById("newSlavePassword").value.trim();

  if (!isValidMAC(slaveMac)) {
    showToast("Adresse MAC invalide (AA:BB:CC:DD:EE:FF)", "error");
    return;
  }

  const data = {
    mac: slaveMac,
    moduleId: slaveId || ("Slave-" + Date.now()),
    wifiSSID: slaveSSID,
    wifiPassword: slavePwd,
  };

  fetch("/api/slaves/add", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  })
    .then(r => r.json())
    .then(res => {
      if (res.status === "ok" || res.status === "already_exists") {
        showToast(res.status === "already_exists" ? "Ce slave existe déjà" : "Slave ajouté !", "success");
        closeAddSlaveModal();
        loadConfiguredSlaves();
      } else {
        showToast(res.error || "Erreur", "error");
      }
    })
    .catch(err => showToast("Erreur réseau", "error"));
}

function deleteSlave(mac) {
  if (!confirm(`Supprimer le slave ${mac} ?`)) return;

  fetch("/api/slaves/delete", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ mac }),
  })
    .then(r => r.json())
    .then(res => {
      showToast(res.status === "ok" ? "Slave supprimé" : (res.error || "Erreur"), res.status === "ok" ? "success" : "error");
      if (res.status === "ok") loadConfiguredSlaves();
    })
    .catch(() => showToast("Erreur réseau", "error"));
}

// ================= PASSAGE =================
function incrementPassage() {
  let currentVal = parseInt(document.getElementById("passageCount").innerText) || 0;
  updateCountOnServer(currentVal + 1);
}

function decrementPassage() {
  let currentVal = parseInt(document.getElementById("passageCount").innerText) || 0;
  if (currentVal > 0) updateCountOnServer(currentVal - 1);
}

function updateCountOnServer(val) {
  document.getElementById("passageCount").innerText = val;
  fetch("/api/count", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ count: val })
  })
    .then(res => res.json())
    .then(() => console.log("Compteur mis à jour :", val))
    .catch(err => console.error("Erreur mise à jour compteur :", err));
}

// ================= RESEAUX =================
function changeNetworkMode() {
  let mode = document.getElementById("modeNet").value;
  let area = document.getElementById("networkArea");

  if (mode === "ap") {
    area.innerHTML = `
  <div>
    <h3 style="margin:25px 0 20px 0;">
      <i class="bi bi-router"></i> 
      Gestion Point d'accès
    </h3>
    <div class="input-group">
      <label><i class="bi bi-wifi"></i> Nom Point d'accès</label>
      <input type="text" id="apSSIDInput" placeholder="ESP32-NETWORK" minlength="4" maxlength="20" required>
    </div>
    <div class="input-group">
      <label><i class="bi bi-lock-fill"></i> Mot de passe</label>
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

    fetch("/api/config/ap")
      .then(res => res.json())
      .then(data => {
        document.getElementById("apSSIDInput").value = data.ssid || "";
        document.getElementById("apPassword").value  = data.password || "";
      })
      .catch(err => console.error("Erreur AP fetch :", err));
  }

  if (mode === "online") {
    area.innerHTML = `
    <div>
      <h3 style="margin:25px 0 20px 0;"><i class="bi bi-globe"></i> Mode En Ligne</h3>
      <div class="input-group">
        <label><i class="bi bi-router"></i> SSID WiFi</label>
        <input type="text" id="wifiSSIDInput" placeholder="Nom WiFi" minlength="4" maxlength="20" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-lock-fill"></i> Mot de passe</label>
        <div class="password-group">
          <input type="password" id="wifiPassword" placeholder="********" minlength="8" maxlength="20" required>
          <i class="bi bi-eye" onclick="togglePassword('wifiPassword', this)"></i>
        </div>
      </div>
      <h3 style="margin:25px 0 20px 0;"><i class="bi bi-broadcast"></i> Serveur MQTT Mosquitto</h3>
      <div class="input-group">
        <label><i class="bi bi-server"></i> Adresse IP du Broker</label>
        <input type="text" id="mqttServerInput" placeholder="192.168.1.2" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-signpost-split"></i> Port du Broker</label>
        <input type="number" id="mqttPortInput" placeholder="1883" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-person-fill"></i> Utilisateur (optionnel)</label>
        <input type="text" id="mqttUserInput" placeholder="Ex: mon_user">
      </div>
      <div class="input-group">
        <label><i class="bi bi-lock-fill"></i> Mot de passe (optionnel)</label>
        <div class="password-group">
          <input type="password" id="mqttPasswordInput" placeholder="********">
          <i class="bi bi-eye" onclick="togglePassword('mqttPasswordInput', this)"></i>
        </div>
      </div>
      <button class="btn" type="submit" onclick="submit_reseau(event)">
        <i class="bi bi-check-circle"></i>
        Confirmer
      </button>
    </div>`;

    Promise.all([
      fetch("/api/wifi").then(res => res.json()),
      fetch("/api/mqtt").then(res => res.json()),
    ])
      .then(([wifiData, mqttData]) => {
        document.getElementById("wifiSSIDInput").value  = wifiData.ssid || "";
        document.getElementById("wifiPassword").value   = wifiData.password || "";
        document.getElementById("mqttServerInput").value = mqttData.server || "192.168.1.2";
        document.getElementById("mqttPortInput").value  = mqttData.port || 1883;
        document.getElementById("mqttUserInput").value  = mqttData.user || "";
        document.getElementById("mqttPasswordInput").value = mqttData.password || "";
      })
      .catch(err => console.error("Erreur WiFi/MQTT fetch :", err));
  }
}

// ================= RESEAUX SUBMIT =================
function submit_reseau(event) {
  event.preventDefault();
  let mode = document.getElementById("modeNet").value;

  if (mode === "ap") {
    let ssid     = document.getElementById("apSSIDInput").value.trim();
    let password = document.getElementById("apPassword").value.trim();
    if (ssid === "" || password === "") { showToast("Veuillez remplir tous les champs", "warning"); return; }
    if (ssid.length < 4 || ssid.length > 20) { showToast("Le nom doit contenir entre 4 et 20 caractères", "warning"); return; }
    if (password.length < 8 || password.length > 20) { showToast("Le mot de passe doit contenir entre 8 et 20 caractères", "warning"); return; }

    fetch("/api/config/ap", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password }),
    })
      .then(res => res.json())
      .then(result => showToast(result.message || "Configuration AP enregistrée", "success"))
      .catch(error => showToast(error.message, "error"));

  } else if (mode === "online") {
    let ssid   = document.getElementById("wifiSSIDInput").value.trim();
    let password = document.getElementById("wifiPassword").value.trim();
    let server = document.getElementById("mqttServerInput").value.trim();
    let port   = parseInt(document.getElementById("mqttPortInput").value) || 1883;
    let user   = document.getElementById("mqttUserInput").value.trim();
    let pass   = document.getElementById("mqttPasswordInput").value.trim();

    if (ssid === "" || password === "" || server === "") { showToast("Veuillez remplir tous les champs", "warning"); return; }
    if (ssid.length < 4 || ssid.length > 20) { showToast("Le nom WiFi doit contenir entre 4 et 20 caractères", "warning"); return; }
    if (password.length < 8 || password.length > 20) { showToast("Le mot de passe WiFi doit contenir entre 8 et 20 caractères", "warning"); return; }

    fetch("/api/wifi", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ ssid, password }) })
      .then(resWifi => {
        if (!resWifi.ok) throw new Error("Échec de la configuration WiFi");
        return fetch("/api/mqtt", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ server, port, user, password: pass }) });
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

  if (mode === "offline") {
    area.innerHTML = `
      <div>
        <h3 style="margin:25px 0 20px 0;"><i class="bi bi-usb-drive"></i> Mise à jour Hors Ligne</h3>
        <div class="input-group">
          <label><i class="bi bi-file-earmark-arrow-up"></i> Sélectionner Firmware</label>
          <input type="file" id="firmwareFile" accept=".bin" required>
        </div>
        <button class="btn" type="button" onclick="uploadOfflineFirmware()">
          <i class="bi bi-upload"></i> Téléverser Firmware
        </button>
      </div>`;
  }

  if (mode === "online") {
    area.innerHTML = `
      <div>
        <h3 style="margin:25px 0 20px 0;"><i class="bi bi-cloud-arrow-down"></i> Mise à jour En Ligne</h3>
        <div class="input-group">
          <label><i class="bi bi-hdd-network"></i> Choisir le module</label>
          <select id="onlineModule">
            <option>Module Master</option>
            <option>Module Slave 1</option>
            <option>Module Slave 2</option>
            <option>Module Slave 3</option>
          </select>
        </div>
        <div class="input-group">
          <label><i class="bi bi-link-45deg"></i> URL Firmware</label>
          <input type="url" id="firmwareUrl" placeholder="https://example.com/firmware.bin" required>
        </div>
        <button class="btn" type="button" onclick="uploadOnlineFirmware()">
          <i class="bi bi-cloud-upload"></i> Télécharger et Téléverser
        </button>
      </div>`;
  }
}

function uploadOfflineFirmware() {
  let file = document.getElementById("firmwareFile").files[0];
  if (!file) { showToast("Veuillez sélectionner un fichier .bin", "warning"); return; }
  if (!file.name.endsWith(".bin")) { showToast("Seuls les fichiers .bin sont acceptés", "error"); return; }

  let formData = new FormData();
  formData.append("firmware", file);

  fetch("/update", { method: "POST", body: formData })
    .then(res => res.json())
    .then(result => showToast(result.message || "Firmware envoyé avec succès", "success"))
    .catch(error => showToast(error.message, "error"));
}

function uploadOnlineFirmware() {
  let url = document.getElementById("firmwareUrl").value.trim();
  if (url === "") { showToast("Veuillez saisir l'adresse du firmware", "warning"); return; }
  if (!url.startsWith("http://") && !url.startsWith("https://")) { showToast("L'adresse doit commencer par http:// ou https://", "error"); return; }

  fetch("http://localhost:3000/update-online", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ module: document.getElementById("onlineModule").value, firmwareUrl: url }),
  })
    .then(res => res.json())
    .then(result => showToast(result.message || "Le téléchargement du firmware a démarré", "success"))
    .catch(error => showToast(error.message, "error"));
}

// ================= PARAMETRE SUB MENU =================
function showSettingPage(pageId, element) {
  document.querySelectorAll(".setting-page").forEach(page => page.classList.remove("active"));
  document.getElementById(pageId).classList.add("active");
  document.querySelectorAll(".settings-btn").forEach(btn => btn.classList.remove("active"));
  element.classList.add("active");
}

// ================= TOGGLE PASSWORD =================
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

// ================= LICENCE =================
function randomPart(length) {
  const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const random = new Uint32Array(length);
  crypto.getRandomValues(random);
  return Array.from(random).map(v => chars[v % chars.length]).join("");
}

function generateLicense() {
  return `SMC-${new Date().getFullYear()}-${randomPart(4)}-${randomPart(4)}-${randomPart(4)}-${randomPart(4)}`;
}

function generateNewLicense() {
  document.getElementById("licenseCode").value = generateLicense();
  document.getElementById("licenseDate").value = new Date().toLocaleString();
}

function copyLicense() {
  const licence = document.getElementById("licenseCode").value;
  if (!licence) { showToast("Aucune licence générée", "warning"); return; }
  navigator.clipboard.writeText(licence);
  showToast("Licence copiée", "success");
}

function sendLicense() {
  const licence = document.getElementById("licenseCode").value;
  if (!licence) { showToast("Générez une licence d'abord", "warning"); return; }

  fetch("http://localhost:3000/licence", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ licence, date: new Date().toISOString() }),
  })
    .then(r => r.json())
    .then(result => showToast(result.message || "Licence envoyée", "success"))
    .catch(error => showToast(error.message, "error"));
}

// ================= SENSOR THRESHOLD =================
window.moduleThresholds = window.moduleThresholds || { "local": 80 };

function loadSensorConfig() {
  fetch("/api/config/sensor")
    .then(res => res.json())
    .then(data => {
      const input  = document.getElementById("seuilInput");
      const slider = document.getElementById("seuilSlider");
      const label  = document.getElementById("seuilValue");
      if (!input) return;
      if (data.seuil !== undefined) {
        window.moduleThresholds["local"] = data.seuil;
        // Only load if select is set to "local"
        const select = document.getElementById("seuilModuleSelect");
        if (!select || select.value === "local") {
          input.value = data.seuil;
          if (slider) { slider.min = data.seuilMin || 10; slider.max = data.seuilMax || 500; slider.value = data.seuil; }
          if (label) label.textContent = data.seuil + " cm";
        }
      }
    })
    .catch(err => console.error("Erreur fetch seuil :", err));
}

function onSeuilModuleChange(val) {
  const threshold = window.moduleThresholds[val] || 80;
  const input  = document.getElementById("seuilInput");
  const slider = document.getElementById("seuilSlider");
  const label  = document.getElementById("seuilValue");
  if (input) input.value = threshold;
  if (slider) slider.value = threshold;
  if (label) label.textContent = threshold + " cm";
}

function syncSeuilSlider(val) {
  const input = document.getElementById("seuilInput");
  const label = document.getElementById("seuilValue");
  if (input) input.value = val;
  if (label) label.textContent = val + " cm";
  
  // Sync to current selection map
  const select = document.getElementById("seuilModuleSelect");
  const target = select ? select.value : "local";
  window.moduleThresholds[target] = parseInt(val);
}

function syncSeuilInput(val) {
  const slider = document.getElementById("seuilSlider");
  const label  = document.getElementById("seuilValue");
  if (slider) slider.value = val;
  if (label)  label.textContent = val + " cm";
  
  // Sync to current selection map
  const select = document.getElementById("seuilModuleSelect");
  const target = select ? select.value : "local";
  window.moduleThresholds[target] = parseInt(val);
}

function saveSensorConfig() {
  const input = document.getElementById("seuilInput");
  if (!input) return;
  const val = parseInt(input.value);
  if (isNaN(val) || val < 10 || val > 500) { showToast("Valeur invalide (10 – 500 cm)", "error"); return; }

  const select = document.getElementById("seuilModuleSelect");
  const target = select ? select.value : "local";

  const payload = { seuil: val };
  if (target !== "local") {
    payload.mac = target;
  }

  fetch("/api/config/sensor", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  })
    .then(res => res.json())
    .then(data => {
      if (data.status === "ok") {
        showToast("Seuil mis à jour : " + val + " cm", "success");
        window.moduleThresholds[target] = val;
      }
      else showToast(data.error || "Erreur serveur", "error");
    })
    .catch(() => showToast("Erreur de connexion", "error"));
}

// ================= DEVICE STATUS =================
function updateDeviceStatus() {
  fetch("/api/status")
    .then(res => res.json())
    .then(data => {
      if ((!websocket || websocket.readyState !== WebSocket.OPEN) && document.getElementById("passageCount")) {
        document.getElementById("passageCount").innerText = data.total || 0;
      }

      if (document.getElementById("deviceMAC"))
        document.getElementById("deviceMAC").innerText = data.mac || "Inconnu";
      if (document.getElementById("deviceRole"))
        document.getElementById("deviceRole").innerText = (data.moduleId ? `${data.moduleId} (${data.role || "neutral"})` : (data.role || "neutral")).toUpperCase();
      if (document.getElementById("deviceMode"))
        document.getElementById("deviceMode").innerText = data.connected ? "En ligne (STA + AP)" : "Hors ligne (AP)";

      let dashMode = document.getElementById("dashMode");
      if (dashMode) {
        dashMode.innerText = data.connected ? "En ligne" : "Hors ligne";
        let icon = dashMode.parentElement.nextElementSibling?.querySelector("i");
        if (icon) {
          if (data.connected) { icon.className = "bi bi-wifi"; icon.parentElement.className = "card-icon green"; }
          else { icon.className = "bi bi-wifi-off"; icon.parentElement.className = "card-icon red"; }
        }
      }
    })
    .catch(err => console.error("Status Error:", err));
}

// ================= MODULES COUNT DASHBOARD =================
const roleColors = {
  master: { bg: "linear-gradient(135deg, #1e40af 0%, #3b82f6 100%)", icon: "bi-star-fill",  badge: "badge-master" },
  slave:  { bg: "linear-gradient(135deg, #065f46 0%, #10b981 100%)", icon: "bi-cpu-fill",   badge: "badge-slave"  },
};

function updateModulesCount() {
  fetch("/api/modules_count")
    .then(res => res.json())
    .then(data => {
      const grid  = document.getElementById("moduleCountsGrid");
      const label = document.getElementById("moduleCountLabel");
      if (!grid) return;

      const modules = data.modules || [];
      const total   = data.total || 0;

      // Track module thresholds globally
      window.moduleThresholds = window.moduleThresholds || {};
      const select = document.getElementById("seuilModuleSelect");
      const currentSelected = select ? select.value : "local";
      const seuilModuleGroup = document.getElementById("seuilModuleGroup");

      if (select) {
        select.innerHTML = '<option value="local">Ce module (Master)</option>';
      }

      // Check if we are running on a slave
      // (If Master, modules[0] is Master, but we can check if there's any slave module)
      let hasSlaves = false;

      // Mise à jour label
      if (label) label.textContent = `${modules.length} module(s) — Total : ${total}`;

      // Mise à jour ou création des cartes
      grid.innerHTML = "";

      modules.forEach((mod, idx) => {
        const colors = roleColors[mod.role] || roleColors.slave;
        const isActive = mod.active !== false;

        // Store threshold
        if (mod.role === "master") {
          window.moduleThresholds["local"] = mod.seuil || 80;
        } else {
          hasSlaves = true;
          window.moduleThresholds[mod.mac] = mod.seuil || 80;
          if (select) {
            const opt = document.createElement("option");
            opt.value = mod.mac;
            opt.textContent = `${mod.moduleId || "Slave"} (${mod.mac})`;
            select.appendChild(opt);
          }
        }

        const card = document.createElement("div");
        card.className = "module-count-card" + (isActive ? "" : " inactive");
        card.id = "mc-" + (mod.mac || idx).replace(/:/g, "");
        card.innerHTML = `
          <div class="mc-header" style="background:${colors.bg}">
            <i class="bi ${colors.icon} mc-icon"></i>
            <div class="mc-badge ${colors.badge}">${mod.role === "master" ? "MASTER" : "SLAVE"}</div>
            ${!isActive ? '<div class="mc-offline"><i class="bi bi-wifi-off"></i></div>' : ""}
          </div>
          <div class="mc-body">
            <div class="mc-name">${mod.moduleId || "Module " + (idx + 1)}</div>
            <div class="mc-count" id="mc-count-${idx}">${mod.count || 0}</div>
            <div class="mc-label">passages détectés</div>
            <div class="mc-mac"><i class="bi bi-cpu"></i> ${mod.mac || "—"}</div>
          </div>
        `;
        grid.appendChild(card);
      });

      if (select) {
        select.value = currentSelected;
      }
      if (seuilModuleGroup) {
        seuilModuleGroup.style.display = hasSlaves ? "block" : "none";
      }

      // Carte total générale
      const totalCard = document.createElement("div");
      totalCard.className = "module-count-card total-card";
      totalCard.innerHTML = `
        <div class="mc-header" style="background: linear-gradient(135deg, #7c3aed 0%, #a78bfa 100%)">
          <i class="bi bi-bar-chart-fill mc-icon"></i>
          <div class="mc-badge" style="background:rgba(255,255,255,0.2)">TOTAL</div>
        </div>
        <div class="mc-body">
          <div class="mc-name">Total Général</div>
          <div class="mc-count" style="color:#a78bfa">${total}</div>
          <div class="mc-label">tous modules confondus</div>
          <div class="mc-mac"><i class="bi bi-people-fill"></i> ${modules.length} module(s) actif(s)</div>
        </div>
      `;
      grid.appendChild(totalCard);

      // Mise à jour du compteur principal
      const passageEl = document.getElementById("passageCount");
      if (passageEl && (!websocket || websocket.readyState !== WebSocket.OPEN)) {
        passageEl.innerText = total;
      }
    })
    .catch(err => console.error("Erreur modules_count :", err));
}

// ================= INIT =================
document.addEventListener("DOMContentLoaded", () => {
  initWebSocket();
  loadModuleConfig();

  fetch("/api/wifi")
    .then(res => res.json())
    .then(wifiData => {
      let modeNetSelect = document.getElementById("modeNet");
      if (modeNetSelect) {
        modeNetSelect.value = (wifiData.ssid && wifiData.ssid.trim() !== "") ? "online" : "ap";
      }
      changeNetworkMode();
    })
    .catch(() => changeNetworkMode());

  changeFirmwareMode();
  animatePageTitle("Dashboard");
  loadSensorConfig();

  // Status périodique (5s)
  updateDeviceStatus();
  setInterval(updateDeviceStatus, 500);

  // Compteurs modules (3s pour plus de réactivité)
  updateModulesCount();
  setInterval(updateModulesCount, 300);
});