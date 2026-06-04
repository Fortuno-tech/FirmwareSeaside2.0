function toggleSidebar() {
  document.getElementById("sidebar").classList.toggle("active");
}

document.addEventListener("click", function (e) {
  let sidebar = document.getElementById("sidebar");
  let burger = document.querySelector(".burger");
  if (window.innerWidth <= 768) {
    if (sidebar && !sidebar.contains(e.target) && burger && !burger.contains(e.target)) {
      sidebar.classList.remove("active");
    }
  }
});

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
  if (window.innerWidth <= 768) document.getElementById("sidebar").classList.remove("active");
}

function changeNetworkMode() {
  let mode = document.getElementById("modeNet").value;
  let area = document.getElementById("networkArea");
  if (mode === "ap") {
    area.innerHTML = '<div><h3 style="margin:25px 0 20px 0;"><i class="bi bi-router"></i> Gestion Point d accès</h3><div class="input-group"><label>SSID</label><input type="text" id="apSSID" minlength="4" maxlength="20" required></div><div class="input-group"><label>MDP</label><input type="password" id="apPassword" minlength="8" maxlength="20" required></div><button class="btn" type="submit">Confirmer</button></div>';
  } else {
    area.innerHTML = '<div><h3>Mode Online</h3><div class="input-group"><label>SSID WiFi</label><input type="text" id="wifiSSID" required></div><div class="input-group"><label>MDP</label><input type="password" id="wifiPass" required></div><button class="btn" type="submit">Confirmer</button></div>';
  }
}

function isValidMAC(mac) {
  let regex = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;
  return regex.test(mac);
}

const moduleForm = document.getElementById("moduleForm");
if(moduleForm) {
    moduleForm.addEventListener("submit", function (e) {
      e.preventDefault();
      let mac1 = document.getElementById("mac1").value.trim();
      let mac2 = document.getElementById("mac2").value.trim();
      if (!isValidMAC(mac1) || !isValidMAC(mac2)) { alert("MAC invalide"); return; }
      
      let type = document.getElementById("typeModule").value;
      fetch("/api/config/module", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ role: type.toLowerCase(), masterMAC: mac2 })
      }).then(() => alert("Config Module Envoyée !"));

      let card = document.createElement("div");
      card.className = "module-card";
      card.innerHTML = "<h4>Module " + type + "</h4><p>Passage : " + passage + "</p>";
      document.getElementById("moduleList").appendChild(card);
      this.reset();
    });
}

let passage = 50;
function incrementPassage() { passage++; document.getElementById("passageCount").innerText = passage; }
function decrementPassage() { if (passage > 0) { passage--; document.getElementById("passageCount").innerText = passage; } }

function submit_reseau(event) {
  if(event) event.preventDefault();
  let ssid = "", pass = "";
  if(document.getElementById("apSSID")) {
    ssid = document.getElementById("apSSID").value;
    pass = document.getElementById("apPassword").value;
  } else if(document.getElementById("wifiSSID")) {
    ssid = document.getElementById("wifiSSID").value;
    pass = document.getElementById("wifiPass").value;
  }
  fetch("/api/config/ap", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ ssid: ssid, password: pass })
  }).then(() => alert("Config Réseau Envoyée !"));
}

function openPasswordModal(el) { document.getElementById("passwordModal").classList.add("active"); window.lastEl = el; }
function confirmPassword() {
  let p = document.getElementById("adminPassword").value;
  if(p === "admin" || p === "1234") {
    document.getElementById("passwordModal").classList.remove("active");
    showPage("parametre", window.lastEl);
    document.getElementById("adminPassword").value = "";
  } else { document.getElementById("passwordError").innerText = "Faux"; }
}
function togglePassword(id, icon) {
  let input = document.getElementById(id);
  input.type = input.type === "password" ? "text" : "password";
}
function showSettingPage(page, el) {
  document.querySelectorAll(".setting-page").forEach(p => p.classList.remove("active"));
  document.getElementById(page).classList.add("active");
  document.querySelectorAll(".settings-btn").forEach(b => b.classList.remove("active"));
  el.classList.add("active");
}
function changeFirmwareMode() {
  let m = document.getElementById("firmwareMode").value;
  let a = document.getElementById("firmwareArea");
  if(m === "offline") a.innerHTML = '<div><input type="file" id="f"><button class="btn" style="margin-top:10px;" onclick="location.href=`/update`">Update</button></div>';
  else a.innerHTML = '<p>Online...</p>';
}

setInterval(() => {
  fetch("/api/status").then(res => res.json()).then(data => {
    passage = data.total || 0;
    const el = document.getElementById("passageCount");
    if(el) el.innerText = passage;
    const cards = document.querySelectorAll(".card h2");
    if(cards.length >= 2) cards[1].innerText = data.role === "master" ? "Master" : "Slave";
  }).catch(() => {});
}, 2000);

changeNetworkMode();
