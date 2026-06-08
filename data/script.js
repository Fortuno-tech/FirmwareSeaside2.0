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

  // Fermer la sidebar sur mobile
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }

  document.getElementById("passwordModal").classList.add("active");

  document.getElementById("adminPassword").value = "";

  document.getElementById("passwordError").innerText = "";

  currentParamElement = element;

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

    Swal.fire({
      icon: "success",
      title: "Accès autorisé",
      text: "Bienvenue dans les paramètres",
      confirmButtonText: "Continuer",
      confirmButtonColor: "#2563eb",
    }).then(() => {
      showPage("parametre", currentParamElement);
    });

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

  <div class="password-group">
    <input
      type="password"
      id="apPassword"
      placeholder="********"
      minlength="8"
      maxlength="20"
      required
    >

    <i
      class="bi bi-eye"
      onclick="togglePassword('apPassword', this)"
    ></i>
  </div>

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

  <div class="password-group">
    <input
      type="password"
      id="wifiPassword"
      placeholder="********"
      minlength="8"
      maxlength="20"
      required
    >

    <i
      class="bi bi-eye"
      onclick="togglePassword('wifiPassword', this)"
    ></i>
  </div>

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

  let type = document.getElementById("typeModule").value;

  let typeEntree = document.getElementById("typeEntree").value;

  if (!isValidMAC(mac1)) {
    Swal.fire({
      icon: "error",
      title: "Adresse MAC invalide",
      text: "Veuillez respecter le format AA:BB:CC:DD:EE:FF",
    });
    return;
  }

  if (!isValidMAC(mac2)) {
    Swal.fire({
      icon: "error",
      title: "Adresse MAC invalide",
      text: "Veuillez vérifier l'adresse MAC du module associé",
    });
    return;
  }

  if (typeEntree === "") {
    Swal.fire({
      icon: "warning",
      title: "Choix requis",
      text: "Veuillez sélectionner un type d'entrée",
    });
    return;
  }
  const data = {
    type: type,
    macMaster: mac1,
    macSlave: mac2,
    typeE: typeEntree,
  };

  fetch("http://localhost:3000/module", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(data),
  })
    .then((response) => response.json())
    .then((result) => {
      Swal.fire({
        icon: "success",
        title: "Module ajouté",
        text: result.message || "Succès",
      });
    })
    .catch((error) => {
      Swal.fire({
        icon: "error",
        title: "Erreur",
        text: error.message,
      });
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
    Swal.fire({
      icon: "warning",
      title: "Formulaire incomplet",
      text: "Veuillez remplir tous les champs",
    });
    return;
  }

  let ssid = inputs[0].value.trim();

  let password = inputs[1].value.trim();

  if (ssid.length < 4 || ssid.length > 20) {
    Swal.fire({
      icon: "warning",
      title: "SSID invalide",
      text: "Le nom doit contenir entre 4 et 20 caractères",
    });

    return;
  }

  if (password.length < 8 || password.length > 20) {
    Swal.fire({
      icon: "warning",
      title: "Mot de passe invalide",
      text: "Le mot de passe doit contenir entre 8 et 20 caractères",
    });

    return;
  }

  const data = {
    mode: mode,
    ssid: ssid,
    password: password,
  };

  fetch("http://localhost:3000/reseau", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(data),
  })
    .then((response) => response.json())
    .then((result) => {
      Swal.fire({
        icon: "success",
        title: "Configuration enregistrée",
        text: result.message || "Succès",
      });
    })
    .catch((error) => {
      Swal.fire({
        icon: "error",
        title: "Erreur",
        text: error.message,
      });
    });

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
    Swal.fire({
      icon: "warning",
      title: "Firmware manquant",
      text: "Veuillez sélectionner un fichier .bin",
    });
    return;
  }

  // vérification extension
  if (!file.name.endsWith(".bin")) {
    Swal.fire({
      icon: "error",
      title: "Fichier invalide",
      text: "Seuls les fichiers .bin sont acceptés",
    });
    return;
  }
  const data = {
    module: module,
    firmwareUrl: url,
  };

  let formData = new FormData();

  formData.append("firmware", file);

  fetch("http://localhost:3000/update-offline", {
    method: "POST",
    body: formData,
  })
    .then((response) => response.json())
    .then((result) => {
      Swal.fire({
        icon: "success",
        title: "Firmware envoyé",
        text: result.message,
      });
    })
    .catch((error) => {
      Swal.fire({
        icon: "error",
        title: "Erreur",
        text: error.message,
      });
    });

  console.log("Firmware sélectionné :", file.name);

  Swal.fire({
    icon: "success",
    title: "Téléversement réussi",
    text: "Le firmware a été envoyé avec succès",
  });
}

// ================= UPLOAD ONLINE =================

function uploadOnlineFirmware() {
  let module = document.getElementById("onlineModule").value;

  let url = document.getElementById("firmwareUrl").value.trim();

  if (url === "") {
    Swal.fire({
      icon: "warning",
      title: "URL manquante",
      text: "Veuillez saisir l'adresse du firmware",
    });
    return;
  }

  // simple validation
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    Swal.fire({
      icon: "error",
      title: "URL invalide",
      text: "L'adresse doit commencer par http:// ou https://",
    });
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
      Swal.fire({
        icon: "success",
        title: "Mise à jour lancée",
        text: result.message,
      });
    })
    .catch((error) => {
      Swal.fire({
        icon: "error",
        title: "Erreur",
        text: error.message,
      });
    });

  /*console.log("Module :", module);

  console.log("Firmware URL :", url);*/

  Swal.fire({
    icon: "success",
    title: "Mise à jour lancée",
    text: "Le téléchargement du firmware a démarré",
  });
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

changeNetworkMode();
