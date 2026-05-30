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

  if (page === "dashboard") {
    title.innerText = "Dashboard";
  }

  if (page === "reseaux") {
    title.innerText = "Réseaux";
  }

  if (page === "module") {
    title.innerText = "Module";
  }

  if (page === "parametre") {
    title.innerText = "Paramètre";
  }

  // mobile
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }
}

// ================= PASSWORD MODAL =================

let currentParamElement = null;

function openPasswordModal(element) {
  currentParamElement = element;

  document.getElementById("passwordModal").classList.add("active");

  document.getElementById("adminPassword").value = "";

  document.getElementById("passwordError").innerText = "";
}

function confirmPassword() {
  let password = document.getElementById("adminPassword").value;

  if (password === "1112") {
    document.getElementById("passwordModal").classList.remove("active");

    showPage("parametre", currentParamElement);

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

          <input
            type="text"
            placeholder="ESP32-NETWORK"
            minlength="4"
            maxlength="20"
            required
          >

        </div>

        <div class="input-group">

          <label>
            <i class="bi bi-lock-fill"></i>
            Mot de passe
          </label>

          <input
            type="password"
            placeholder="********"
            minlength="8"
            maxlength="20"
            required
          >

        </div>

        <button class="btn" type="submit">
          <i class="bi bi-check-circle"></i>
          Confirmer
        </button>

      </div>

    `;
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

          <input
            type="text"
            placeholder="Nom WiFi"
            minlength="4"
            maxlength="20"
            required
          >

        </div>

        <div class="input-group">

          <label>
            <i class="bi bi-lock-fill"></i>
            Mot de passe
          </label>

          <input
            type="password"
            placeholder="********"
            minlength="8"
            maxlength="20"
            required
          >

        </div>

        <button class="btn" type="submit">
          <i class="bi bi-check-circle"></i>
          Confirmer
        </button>

      </div>

    `;
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

  if (!isValidMAC(mac1)) {
    alert("Adresse MAC invalide");
    return;
  }

  if (!isValidMAC(mac2)) {
    alert("Adresse MAC autre module invalide");
    return;
  }

  let type = document.getElementById("typeModule").value;

  let card = document.createElement("div");

  card.className = "module-card";

  card.innerHTML = `

      <h4>
        <i class="bi bi-hdd-network"></i>
        Module ${type}
      </h4>

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

// ================= PASSAGE =================

let passage = 50;

function incrementPassage() {
  passage++;

  document.getElementById("passageCount").innerText = passage;
}

function decrementPassage() {
  if (passage > 0) {
    passage--;

    document.getElementById("passageCount").innerText = passage;
  }
}

// ================= RESEAUX SUBMIT =================

function submit_reseau(event) {
  event.preventDefault();

  let mode = document.getElementById("modeNet").value;

  let area = document.getElementById("networkArea");

  let inputs = area.querySelectorAll("input");

  if (inputs.length < 2) {
    alert("Formulaire incomplet");
    return;
  }

  let ssid = inputs[0].value.trim();

  let password = inputs[1].value.trim();

  if (ssid.length < 4 || ssid.length > 20) {
    alert("SSID invalide");
    return;
  }

  if (password.length < 8 || password.length > 20) {
    alert("Mot de passe invalide");
    return;
  }

  if (mode === "ap") {
    alert("Configuration Point d'accès enregistrée !");
  }

  if (mode === "online") {
    alert("Configuration WiFi enregistrée !");
  }

  inputs.forEach((input) => {
    input.value = "";
  });
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
    alert("Veuillez sélectionner un firmware");
    return;
  }

  // vérification extension
  if (!file.name.endsWith(".bin")) {
    alert("Le fichier doit être en .bin");
    return;
  }

  console.log("Firmware sélectionné :", file.name);

  alert("Firmware téléversé avec succès en mode hors ligne !");
}

// ================= UPLOAD ONLINE =================

function uploadOnlineFirmware() {
  let module = document.getElementById("onlineModule").value;

  let url = document.getElementById("firmwareUrl").value.trim();

  if (url === "") {
    alert("Veuillez saisir une URL firmware");
    return;
  }

  // simple validation
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    alert("URL invalide");
    return;
  }

  console.log("Module :", module);

  console.log("Firmware URL :", url);

  alert("Téléchargement et téléversement du firmware lancé !");
}

// ================= DEFAULT FIRMWARE =================

changeFirmwareMode();
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

changeNetworkMode();
