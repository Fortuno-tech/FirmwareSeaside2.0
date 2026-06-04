// ================= SIDEBAR =================
function toggleSidebar() {
  document.getElementById("sidebar").classList.toggle("active");
}

// fermer sidebar quand on clique dehors
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

  // changer titre
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

  // fermer sidebar mobile
  if (window.innerWidth <= 768) {
    document.getElementById("sidebar").classList.remove("active");
  }
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

                            <input type="text"
                                placeholder="ESP32-NETWORK"
                                minlength="4"
                                maxlength="20"
                                required>

                        </div>

                        <div class="input-group">

                            <label>
                                <i class="bi bi-lock-fill"></i>
                                Mot de passe
                            </label>

                            <input type="password"
                                placeholder="********"
                                minlength="8"
                                maxlength="20"
                                required>

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

                            <input type="text"
                                placeholder="Nom WiFi"
                                minlength="4"
                                maxlength="20"
                                required>

                        </div>

                        <div class="input-group">

                            <label>
                                <i class="bi bi-lock-fill"></i>
                                Mot de passe
                            </label>

                            <input type="password"
                                placeholder="********"
                                minlength="8"
                                maxlength="20"
                                required>

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

  // validation
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

// ================= COMPTEUR PASSAGE =================
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
function submit_reseau() {
  event.preventDefault(); // empêche refresh

  let mode = document.getElementById("modeNet").value;
  let area = document.getElementById("networkArea");

  let inputs = area.querySelectorAll("input");

  // sécurisation
  if (inputs.length < 2) {
    alert("Formulaire incomplet");
    return;
  }

  let ssid = inputs[0].value.trim();
  let password = inputs[1].value.trim();

  // ================= VALIDATION =================
  if (ssid.length < 4 || ssid.length > 20) {
    alert("SSID / Nom réseau invalide (4 à 20 caractères)");
    return;
  }

  if (password.length < 8 || password.length > 20) {
    alert("Mot de passe invalide (8 à 20 caractères)");
    return;
  }

  // ================= RESULTAT =================
  if (mode === "ap") {
    console.log("MODE POINT D'ACCÈS");
    console.log("SSID :", ssid);
    console.log("PASSWORD :", password);

    alert("Configuration Point d'accès enregistrée !");
  }

  if (mode === "online") {
    console.log("MODE EN LIGNE");
    console.log("SSID WiFi :", ssid);
    console.log("PASSWORD :", password);

    alert("Configuration WiFi enregistrée !");
  }

  // reset visuel
  area.querySelectorAll("input").forEach((input) => (input.value = ""));
}

// ================= DEFAULT =================
changeNetworkMode();
