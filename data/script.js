// =====================================================
// SmartCount - script.js
// =====================================================

const $ = (id) => document.getElementById(id);
const esc = (s) =>
  String(s ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
const cap = (s) => (s ? s.charAt(0).toUpperCase() + s.slice(1) : s);

// Désactive un bouton et affiche un indicateur pendant une requête
function track(btn, promise) {
  if (!btn) return promise;
  btn.disabled = true;
  btn.classList.add("loading");
  return promise.finally(() => {
    btn.disabled = false;
    btn.classList.remove("loading");
  });
}

// =====================================================
// DROPDOWN PERSONNALISÉ (remplace tous les <select>)
//
// Markup :
//   <div class="dd" id="monChamp" data-value="b" data-placeholder="Choisir"
//        data-onchange="maFonction" data-searchable="auto|true|false">
//     <span data-value="a">Option A</span>
//     <span data-value="b" data-hint="info" data-disabled>Option B</span>
//   </div>
//
// API :  ddValue(id)  ddSet(id, valeur)  Dropdown.get(id).setOptions([{value,label,hint,disabled}])
//        Dropdown.init(racine)  à appeler après avoir injecté du HTML dynamique
// =====================================================
const isSheet = () => window.matchMedia("(max-width: 600px)").matches;

class Dropdown {
  constructor(el) {
    this.el = el;
    el._dd = this;
    this.uid = el.id || "dd" + ++Dropdown.count;
    this.placeholder = el.dataset.placeholder || "";
    this.options = Array.from(el.children)
      .filter((n) => n.dataset.value !== undefined)
      .map((n) => ({
        value: n.dataset.value,
        label: n.textContent.trim(),
        hint: n.dataset.hint || "",
        disabled: n.hasAttribute("data-disabled"),
      }));
    this.value =
      el.dataset.value !== undefined ? el.dataset.value : this.placeholder ? "" : this.options[0] ? this.options[0].value : "";
    this.isOpen = false;
    this.active = -1;
    this.query = "";
    this.visible = [];
    this.sig = "";
    this.build();
    this.updateLabel();
  }

  build() {
    const el = this.el;
    el.classList.add("dd");
    el.innerHTML = `
      <button type="button" class="dd-trigger" aria-haspopup="listbox" aria-expanded="false" aria-controls="${this.uid}-list">
        <span class="dd-label" id="${this.uid}-value"></span>
        <i class="bi bi-chevron-down dd-caret" aria-hidden="true"></i>
      </button>
      <div class="dd-menu">
        <div class="dd-search" hidden>
          <i class="bi bi-search" aria-hidden="true"></i>
          <input type="text" placeholder="Rechercher" autocomplete="off" aria-label="Rechercher" />
        </div>
        <ul class="dd-list" role="listbox" id="${this.uid}-list" tabindex="-1"></ul>
      </div>`;
    this.trigger = el.querySelector(".dd-trigger");
    this.label = el.querySelector(".dd-label");
    this.menu = el.querySelector(".dd-menu");
    this.searchBox = el.querySelector(".dd-search");
    this.input = el.querySelector(".dd-search input");
    this.list = el.querySelector(".dd-list");

    const lab = el.closest(".input-group") && el.closest(".input-group").querySelector("label");
    if (lab) {
      lab.id = lab.id || this.uid + "-label";
      this.trigger.setAttribute("aria-labelledby", lab.id + " " + this.uid + "-value");
    }

    this.trigger.addEventListener("click", () => this.toggle());
    this.list.addEventListener("click", (e) => {
      const li = e.target.closest(".dd-option");
      if (li) this.choose(+li.dataset.index);
    });
    this.list.addEventListener("mousemove", (e) => {
      const li = e.target.closest(".dd-option:not(.disabled)");
      if (li && +li.dataset.index !== this.active) this.setActive(+li.dataset.index, false);
    });
    this.input.addEventListener("input", () => {
      this.query = this.input.value.trim().toLowerCase();
      this.active = -1;
      this.renderList();
    });
    el.addEventListener("keydown", (e) => this.onKey(e));
    el.addEventListener("keyup", (e) => {
      if (e.key === " " && e.target === this.trigger) e.preventDefault(); // évite un clic parasite
    });
  }

  // ---------- état ----------
  updateLabel() {
    const o = this.options.find((x) => x.value === this.value);
    this.label.textContent = o ? o.label : this.placeholder;
    this.label.classList.toggle("placeholder", !o);
    this.trigger.title = o ? o.label : "";
  }

  setValue(v) {
    v = String(v);
    if (!this.options.some((o) => o.value === v)) return false;
    this.value = v;
    this.updateLabel();
    if (this.isOpen) this.renderList();
    return true;
  }

  setOptions(opts) {
    const sig = JSON.stringify(opts);
    if (sig === this.sig) return;
    this.sig = sig;
    this.options = opts.map((o) => ({ hint: "", disabled: false, ...o, value: String(o.value) }));
    if (!this.options.some((o) => o.value === this.value)) {
      this.value = this.placeholder ? "" : this.options[0] ? this.options[0].value : "";
    }
    this.updateLabel();
    if (this.isOpen) this.renderList();
  }

  // ---------- rendu ----------
  renderList() {
    const q = this.query;
    this.visible = [];
    this.options.forEach((o, i) => {
      if (!q || (o.label + " " + o.hint).toLowerCase().includes(q)) this.visible.push(i);
    });

    if (!this.visible.length) {
      this.list.innerHTML = `<li class="dd-empty" role="presentation">${
        this.options.length ? "Aucun résultat" : "Aucune option disponible"
      }</li>`;
      this.active = -1;
      this.markActive(false);
      return;
    }

    this.list.innerHTML = this.visible
      .map((i) => {
        const o = this.options[i];
        const sel = o.value === this.value;
        return `<li class="dd-option${sel ? " selected" : ""}${o.disabled ? " disabled" : ""}" role="option" id="${this.uid}-o${i}" data-index="${i}" aria-selected="${sel}"${o.disabled ? ' aria-disabled="true"' : ""}>
          <span class="dd-text">${esc(o.label)}${o.hint ? `<small class="dd-hint">${esc(o.hint)}</small>` : ""}</span>
          <i class="bi bi-check2 dd-check" aria-hidden="true"></i>
        </li>`;
      })
      .join("");

    if (!this.visible.includes(this.active) || this.options[this.active].disabled) {
      const sel = this.visible.find((i) => this.options[i].value === this.value && !this.options[i].disabled);
      const first = this.visible.find((i) => !this.options[i].disabled);
      this.active = sel !== undefined ? sel : first !== undefined ? first : -1;
    }
    this.markActive(false);
  }

  markActive(scroll) {
    let current = null;
    this.list.querySelectorAll(".dd-option").forEach((li) => {
      const on = +li.dataset.index === this.active;
      li.classList.toggle("active", on);
      if (on) current = li;
    });
    [this.trigger, this.input].forEach((n) => {
      if (current) n.setAttribute("aria-activedescendant", current.id);
      else n.removeAttribute("aria-activedescendant");
    });
    if (current && scroll) current.scrollIntoView({ block: "nearest" });
  }

  setActive(i, scroll = true) {
    this.active = i;
    this.markActive(scroll);
  }

  move(dir) {
    const enabled = this.visible.filter((i) => !this.options[i].disabled);
    if (!enabled.length) return;
    let pos = enabled.indexOf(this.active);
    pos = pos === -1 ? (dir > 0 ? 0 : enabled.length - 1) : (pos + dir + enabled.length) % enabled.length;
    this.setActive(enabled[pos]);
  }

  // ---------- ouverture / fermeture ----------
  show() {
    if (this.isOpen) return;
    Dropdown.hideAll();
    this.isOpen = true;
    this.query = "";
    this.input.value = "";

    const mode = this.el.dataset.searchable;
    const useSearch = mode === "true" || (mode !== "false" && this.options.length >= 8);
    this.searchBox.hidden = !useSearch;

    this.active = this.options.findIndex((o) => o.value === this.value && !o.disabled);
    this.renderList();
    this.el.classList.add("open");
    this.trigger.setAttribute("aria-expanded", "true");
    this.place();
    if (isSheet()) Dropdown.setBackdrop(true);
    this.markActive(true);
    if (useSearch && !isSheet()) this.input.focus({ preventScroll: true });
  }

  hide(refocus) {
    if (!this.isOpen) return;
    this.isOpen = false;
    this.el.classList.remove("open");
    this.trigger.setAttribute("aria-expanded", "false");
    Dropdown.setBackdrop(false);
    if (refocus) this.trigger.focus({ preventScroll: true });
  }

  toggle() {
    this.isOpen ? this.hide() : this.show();
  }

  // Le menu est en position fixed : il n'est jamais coupé par un conteneur
  place() {
    const s = this.menu.style;
    if (isSheet()) {
      s.cssText = "";
      return;
    }
    const r = this.trigger.getBoundingClientRect();
    const vh = window.innerHeight;
    const below = vh - r.bottom - 12;
    const above = r.top - 12;
    const up = below < 180 && above > below;
    s.left = r.left + "px";
    s.width = r.width + "px";
    s.maxHeight = Math.max(120, Math.min(300, up ? above : below)) + "px";
    if (up) {
      s.top = "auto";
      s.bottom = vh - r.top + 4 + "px";
    } else {
      s.bottom = "auto";
      s.top = r.bottom + 4 + "px";
    }
  }

  choose(i) {
    const o = this.options[i];
    if (!o || o.disabled) return;
    const changed = o.value !== this.value;
    this.value = o.value;
    this.updateLabel();
    this.hide(true);
    if (changed) this.emit();
  }

  emit() {
    this.el.dispatchEvent(new CustomEvent("change", { bubbles: true, detail: { value: this.value } }));
    const fn = this.el.dataset.onchange;
    if (fn && typeof window[fn] === "function") window[fn](this.value, this);
  }

  // ---------- clavier ----------
  onKey(e) {
    const k = e.key;
    if (!this.isOpen) {
      if (e.target === this.trigger && (k === "ArrowDown" || k === "ArrowUp" || k === "Enter" || k === " ")) {
        e.preventDefault();
        this.show();
      }
      return;
    }
    const inSearch = e.target === this.input;
    const enabled = this.visible.filter((i) => !this.options[i].disabled);
    switch (k) {
      case "ArrowDown":
        e.preventDefault();
        this.move(1);
        break;
      case "ArrowUp":
        e.preventDefault();
        this.move(-1);
        break;
      case "Home":
        if (!inSearch && enabled.length) {
          e.preventDefault();
          this.setActive(enabled[0]);
        }
        break;
      case "End":
        if (!inSearch && enabled.length) {
          e.preventDefault();
          this.setActive(enabled[enabled.length - 1]);
        }
        break;
      case "Enter":
        e.preventDefault();
        if (this.active >= 0) this.choose(this.active);
        break;
      case " ":
        if (!inSearch) {
          e.preventDefault();
          if (this.active >= 0) this.choose(this.active);
        }
        break;
      case "Escape":
        e.preventDefault();
        e.stopPropagation();
        this.hide(true);
        break;
      case "Tab":
        this.hide();
        break;
    }
  }
}

Dropdown.count = 0;
Dropdown.instances = new Set();
Dropdown.bd = null;

Dropdown.init = (root = document) => {
  Dropdown.instances.forEach((d) => {
    if (!d.el.isConnected) Dropdown.instances.delete(d);
  });
  root.querySelectorAll(".dd").forEach((el) => {
    if (!el._dd) Dropdown.instances.add(new Dropdown(el));
  });
};
Dropdown.get = (id) => {
  const el = $(id);
  return el ? el._dd : null;
};
Dropdown.hideAll = () => Dropdown.instances.forEach((d) => d.hide());
Dropdown.setBackdrop = (on) => {
  if (!Dropdown.bd) {
    if (!on) return;
    const b = document.createElement("div");
    b.className = "dd-backdrop";
    b.addEventListener("click", () => Dropdown.hideAll());
    document.body.appendChild(b);
    Dropdown.bd = b;
  }
  Dropdown.bd.classList.toggle("active", on);
};

document.addEventListener("click", (e) => {
  Dropdown.instances.forEach((d) => {
    if (d.isOpen && !d.el.contains(e.target)) d.hide();
  });
});
const repositionDropdowns = () => Dropdown.instances.forEach((d) => d.isOpen && d.place());
window.addEventListener("resize", repositionDropdowns);
window.addEventListener("scroll", repositionDropdowns, true);

const ddValue = (id) => {
  const d = Dropdown.get(id);
  return d ? d.value : "";
};
const ddSet = (id, v) => {
  const d = Dropdown.get(id);
  return d ? d.setValue(v) : false;
};

// =====================================================
// SIDEBAR
// =====================================================
function setSidebar(open) {
  $("sidebar").classList.toggle("active", open);
  $("sidebarBackdrop").classList.toggle("active", open);
}

function toggleSidebar() {
  setSidebar(!$("sidebar").classList.contains("active"));
}

// =====================================================
// TOASTS
// =====================================================
function showToast(message, type = "success") {
  const colors = { success: "#12805c", error: "#c4322f", warning: "#b5650d", info: "#475467" };
  Toastify({
    text: message,
    duration: 3000,
    gravity: "top",
    position: "right",
    close: true,
    stopOnFocus: true,
    style: {
      background: colors[type] || colors.info,
      color: "#fff",
      borderRadius: "8px",
      boxShadow: "0 8px 20px rgba(16,24,40,.15)",
    },
  }).showToast();
}

// =====================================================
// NAVIGATION
// =====================================================
const PAGES = {
  dashboard: ["Tableau de bord", "Vue d'ensemble des passages et de l'état du module"],
  reseaux: ["Réseaux", "Mode de connexion, Wi-Fi et serveur MQTT"],
  module: ["Module", "Rôle, identité et modules esclaves"],
  licence: ["Licence", "Génération et envoi de la licence au module"],
  parametre: ["Paramètres", "Mises à jour, caractéristiques et capteur"],
};

function showPage(page, element) {
  document.querySelectorAll(".page").forEach((p) => p.classList.remove("active"));
  $(page).classList.add("active");
  document.querySelectorAll(".menu-item").forEach((l) => l.classList.remove("active"));
  if (element) element.classList.add("active");

  const info = PAGES[page] || [page, ""];
  $("pageTitle").textContent = info[0];
  $("pageSubtitle").textContent = info[1];

  setSidebar(false);
  Dropdown.hideAll();

  if (page === "module") {
    loadModuleConfig();
    loadConfiguredSlaves();
  }
}

// =====================================================
// MODALES
// =====================================================
let currentAccessElement = null;
let currentAccessTarget = "parametre";

function openPasswordModal(element, targetPage = "parametre") {
  currentAccessElement = element;
  currentAccessTarget = targetPage;

  $("passwordModalTitleText").textContent = targetPage === "licence" ? "Accès Licence" : "Accès Paramètres";
  $("passwordModalSubtitle").textContent =
    targetPage === "licence"
      ? "Saisissez le mot de passe pour accéder à la licence"
      : "Veuillez saisir le mot de passe";

  setSidebar(false);
  $("passwordModal").classList.add("active");
  $("adminPassword").value = "";
  $("passwordError").innerText = "";
  setTimeout(() => $("adminPassword").focus(), 100);
}

function confirmPassword() {
  if ($("adminPassword").value === "1112") {
    $("passwordModal").classList.remove("active");
    showToast("Accès autorisé", "success");
    setTimeout(() => showPage(currentAccessTarget, currentAccessElement), 300);
    return;
  }
  $("passwordError").innerText = "Mot de passe incorrect";
}

$("adminPassword").addEventListener("keydown", (e) => {
  if (e.key === "Enter") confirmPassword();
});

$("passwordModal").addEventListener("click", function (e) {
  if (e.target === this) this.classList.remove("active");
});

function openAddSlaveModal() {
  ["newSlaveId", "newSlaveMAC", "newSlaveSSID", "newSlavePassword"].forEach((id) => ($(id).value = ""));

  // Pré-remplit les identifiants Wi-Fi du Master
  fetch("/api/config/ap")
    .then((res) => res.json())
    .then((data) => {
      if (data.ssid) $("newSlaveSSID").value = data.ssid;
      if (data.password) $("newSlavePassword").value = data.password;
    })
    .catch((err) => console.error("Erreur récupération config AP master :", err));

  $("addSlaveModal").classList.add("active");
  setTimeout(() => $("newSlaveId").focus(), 100);
}

function closeAddSlaveModal() {
  $("addSlaveModal").classList.remove("active");
}

$("addSlaveModal").addEventListener("click", function (e) {
  if (e.target === this) closeAddSlaveModal();
});

document.addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  $("passwordModal").classList.remove("active");
  closeAddSlaveModal();
});

// =====================================================
// WEBSOCKET
// =====================================================
const gateway = "ws://" + window.location.hostname + "/ws";
let websocket;

function initWebSocket() {
  websocket = new WebSocket(gateway);
  websocket.onopen = () => console.log("Connexion WebSocket établie");
  websocket.onclose = () => {
    console.log("Connexion WebSocket fermée");
    setTimeout(initWebSocket, 2000);
  };
  websocket.onmessage = onMessage;
}

function onMessage(event) {
  const el = $("passageCount");
  if (!el) return;
  const total = normalizeCount(event.data);
  if (total !== null) el.innerText = total;
}

// Le firmware récent envoie {"total":0}, les anciennes versions envoyaient 0.
function normalizeCount(value) {
  let candidate = value;
  if (typeof candidate === "string") {
    try {
      candidate = JSON.parse(candidate);
    } catch (_) {
      /* valeur numérique brute */
    }
  }
  if (candidate && typeof candidate === "object" && typeof candidate.total !== "undefined") {
    candidate = candidate.total;
  }
  const number = Number(candidate);
  return Number.isFinite(number) ? Math.max(0, Math.trunc(number)) : null;
}

// =====================================================
// VALIDATION MAC
// =====================================================
function isValidMAC(mac) {
  return /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/.test(mac);
}

// =====================================================
// MODULE : liaison à un Master
// =====================================================
function toggleLinkToMasterFields() {
  const isChecked = $("linkToMasterCheckbox").checked;
  const section = $("slaveWifiSection");
  if (!section) return;
  section.style.display = isChecked ? "block" : "none";
  $("mac2").required = isChecked;
  $("slaveMasterSSID").required = isChecked;
  $("slaveMasterPassword").required = isChecked;
}

$("moduleForm").addEventListener("submit", function (e) {
  e.preventDefault();

  const mac1 = $("mac1").value.trim();
  const modId = $("moduleIdInput").value.trim();
  const typeEntree = ddValue("typeEntree");
  const isSlave = $("linkToMasterCheckbox").checked;
  const isAlreadySlave = $("unlinkMasterBtn").style.display !== "none";

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
    const mac2 = $("mac2").value.trim();
    if (!isValidMAC(mac2)) {
      showToast("Adresse MAC du Master invalide", "error");
      return;
    }
    data.macSlave = mac2; // Le firmware associe macSlave au masterMAC dans le stockage
    data.wifiSSID = $("slaveMasterSSID").value.trim();
    data.wifiPassword = $("slaveMasterPassword").value.trim();

    if (data.wifiSSID === "") {
      showToast("Veuillez entrer le SSID Wi-Fi du Master", "warning");
      return;
    }
    if (data.wifiPassword.length < 8) {
      showToast("Le mot de passe Wi-Fi du Master doit faire au moins 8 caractères", "warning");
      return;
    }
  } else {
    data.macSlave = "00:00:00:00:00:00";
    data.wifiSSID = "";
    data.wifiPassword = "";
  }

  showToast("Envoi de la configuration...", "info");

  track(
    e.submitter,
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
          showToast("Erreur lors de la configuration : " + error.message, "error");
        }
      })
  );
});

function unlinkFromMaster() {
  if (!confirm("Délier ce module du Master et le faire repasser en Master autonome ? Le module va redémarrer.")) return;

  const data = {
    moduleId: "Master",
    macSlave: "00:00:00:00:00:00",
    wifiSSID: "",
    wifiPassword: "",
    typeE: ddValue("typeEntree") || "vip",
  };

  showToast("Envoi de la commande de déliaison...", "info");

  fetch("/api/config/module", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  })
    .then(() => showToast("Module délié. Redémarrage...", "success"))
    .catch(() => showToast("Module délié. Redémarrage...", "success"));
}

function loadModuleConfig() {
  fetch("/api/config/module")
    .then((res) => res.json())
    .then((data) => {
      const isSlave = data.role && data.role.toLowerCase() === "slave";

      if (data.moduleId) $("moduleIdInput").value = data.moduleId;
      if (data.masterMAC) $("mac2").value = data.masterMAC;
      if (data.wifiSSID) $("slaveMasterSSID").value = data.wifiSSID;
      if (data.wifiPassword) $("slaveMasterPassword").value = data.wifiPassword;

      const toggleGroup = $("linkToMasterToggleGroup");
      const unlinkBtn = $("unlinkMasterBtn");
      const wifiSection = $("slaveWifiSection");
      const slavesConfigSection = $("slavesConfigSection");

      if (isSlave) {
        if (toggleGroup) toggleGroup.style.display = "none";
        if (unlinkBtn) unlinkBtn.style.display = "flex";
        if (wifiSection) {
          wifiSection.style.display = "block";
          $("mac2").disabled = true;
          $("slaveMasterSSID").disabled = true;
          $("slaveMasterPassword").disabled = true;
        }
        if (slavesConfigSection) slavesConfigSection.style.display = "none";
      } else {
        if (toggleGroup) toggleGroup.style.display = "block";
        if (unlinkBtn) unlinkBtn.style.display = "none";
        $("linkToMasterCheckbox").checked = false;
        if (wifiSection) {
          wifiSection.style.display = "none";
          $("mac2").disabled = false;
          $("slaveMasterSSID").disabled = false;
          $("slaveMasterPassword").disabled = false;
        }
        if (slavesConfigSection) slavesConfigSection.style.display = "block";
      }

      const roleBannerText = $("roleBannerText");
      const roleBanner = $("roleBanner");
      if (roleBannerText && data.role) {
        const roleLabels = { master: "Ce module est le Master", slave: "Ce module est un Slave" };
        const role = data.role.toLowerCase();
        roleBannerText.textContent = (data.moduleId ? `${data.moduleId} — ` : "") + (roleLabels[role] || cap(data.role));
        if (roleBanner) roleBanner.className = "role-banner role-" + role;
      }
    })
    .catch((err) => console.error("Erreur module config fetch :", err));

  // Remplit automatiquement l'adresse MAC locale (lecture seule)
  fetch("/api/status")
    .then((res) => res.json())
    .then((data) => {
      const mac1Input = $("mac1");
      if (mac1Input) {
        mac1Input.value = data.mac || "";
        mac1Input.readOnly = true;
      }
    })
    .catch((err) => console.error("Erreur status fetch :", err));
}

// =====================================================
// SLAVES CONFIGURÉS
// =====================================================
function loadConfiguredSlaves() {
  const container = $("moduleList");
  if (!container) return;
  container.innerHTML = "";

  Promise.all([
    fetch("/api/slaves/configured").then((r) => r.json()).catch(() => []),
    fetch("/api/slaves").then((r) => r.json()).catch(() => []),
  ]).then(([configured, discovered]) => {
    const allMACs = new Set(configured.map((s) => s.mac.toUpperCase()));
    const extra = discovered.filter((s) => !allMACs.has(s.mac.toUpperCase()));
    const allSlaves = [...configured, ...extra];

    if (allSlaves.length === 0) {
      container.innerHTML =
        "<p class='empty-slaves'>Aucun module esclave.<br>Cliquez sur <strong>Ajouter un slave</strong> pour en configurer un.</p>";
      return;
    }

    allSlaves.forEach((slave) => {
      const isDiscovered = !allMACs.has(slave.mac.toUpperCase());
      const card = document.createElement("div");
      card.className = "module-card";
      card.innerHTML = `
        <div class="module-card-header">
          <div class="module-card-icon"><i class="bi bi-cpu-fill"></i></div>
          <div class="module-card-title">
            <h4>${esc(slave.moduleId || "Slave")}</h4>
            <span class="module-card-tag ${isDiscovered ? "tag-auto" : "tag-manual"}">${isDiscovered ? "Auto-découvert" : "Configuré"}</span>
          </div>
          <button type="button" class="btn-remove-slave" onclick="deleteSlave('${esc(slave.mac)}')" title="Supprimer" aria-label="Supprimer ce slave">
            <i class="bi bi-x-lg"></i>
          </button>
        </div>
        <div class="module-card-info">
          <p><i class="bi bi-cpu"></i>MAC : <code>${esc(slave.mac)}</code></p>
          ${slave.wifiSSID ? `<p><i class="bi bi-wifi"></i>SSID : ${esc(slave.wifiSSID)}</p>` : ""}
          ${slave.ip ? `<p><i class="bi bi-link-45deg"></i>IP : <a href="http://${esc(slave.ip)}" target="_blank" rel="noopener">${esc(slave.ip)}</a></p>` : ""}
        </div>`;
      container.appendChild(card);
    });
  });
}

function confirmAddSlave() {
  const slaveId = $("newSlaveId").value.trim();
  const slaveMac = $("newSlaveMAC").value.trim();

  if (!isValidMAC(slaveMac)) {
    showToast("Adresse MAC invalide (AA:BB:CC:DD:EE:FF)", "error");
    return;
  }

  const data = {
    mac: slaveMac,
    moduleId: slaveId || "Slave-" + Date.now(),
    wifiSSID: $("newSlaveSSID").value.trim(),
    wifiPassword: $("newSlavePassword").value.trim(),
  };

  fetch("/api/slaves/add", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  })
    .then((r) => r.json())
    .then((res) => {
      if (res.status === "ok" || res.status === "already_exists") {
        showToast(res.status === "already_exists" ? "Ce slave existe déjà" : "Slave ajouté", "success");
        closeAddSlaveModal();
        loadConfiguredSlaves();
      } else {
        showToast(res.error || "Erreur", "error");
      }
    })
    .catch(() => showToast("Erreur réseau", "error"));
}

function deleteSlave(mac) {
  if (!confirm(`Supprimer le slave ${mac} ?`)) return;

  fetch("/api/slaves/delete", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ mac }),
  })
    .then((r) => r.json())
    .then((res) => {
      const ok = res.status === "ok";
      showToast(ok ? "Slave supprimé" : res.error || "Erreur", ok ? "success" : "error");
      if (ok) loadConfiguredSlaves();
    })
    .catch(() => showToast("Erreur réseau", "error"));
}

// =====================================================
// COMPTEUR DE PASSAGES
// =====================================================
function incrementPassage() {
  updateCountOnServer((parseInt($("passageCount").innerText) || 0) + 1);
}

function decrementPassage() {
  const current = parseInt($("passageCount").innerText) || 0;
  if (current > 0) updateCountOnServer(current - 1);
}

function updateCountOnServer(val) {
  $("passageCount").innerText = val;
  fetch("/api/count", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ count: val }),
  })
    .then((res) => res.json())
    .catch((err) => console.error("Erreur mise à jour compteur :", err));
}

// =====================================================
// RÉSEAUX
// =====================================================
function changeNetworkMode() {
  const mode = ddValue("modeNet");
  const area = $("networkArea");

  if (mode === "ap") {
    area.innerHTML = `
      <h3 class="area-title"><i class="bi bi-router"></i>Point d'accès</h3>
      <div class="input-group">
        <label><i class="bi bi-wifi"></i>Nom du point d'accès</label>
        <input type="text" id="apSSIDInput" placeholder="ESP32-NETWORK" minlength="4" maxlength="20" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-lock-fill"></i>Mot de passe</label>
        <div class="password-group">
          <input type="password" id="apPassword" placeholder="********" minlength="8" maxlength="20" required>
          <i class="bi bi-eye" onclick="togglePassword('apPassword', this)"></i>
        </div>
      </div>
      <button class="btn" type="submit" onclick="submit_reseau(event)">
        <i class="bi bi-check-circle"></i>Confirmer
      </button>`;

    fetch("/api/config/ap")
      .then((res) => res.json())
      .then((data) => {
        $("apSSIDInput").value = data.ssid || "";
        $("apPassword").value = data.password || "";
      })
      .catch((err) => console.error("Erreur AP fetch :", err));
  }

  if (mode === "online") {
    area.innerHTML = `
      <h3 class="area-title"><i class="bi bi-globe"></i>Connexion Wi-Fi</h3>
      <div class="input-group">
        <label><i class="bi bi-router"></i>SSID Wi-Fi</label>
        <input type="text" id="wifiSSIDInput" placeholder="Nom du Wi-Fi" minlength="4" maxlength="20" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-lock-fill"></i>Mot de passe</label>
        <div class="password-group">
          <input type="password" id="wifiPassword" placeholder="********" minlength="8" maxlength="20" required>
          <i class="bi bi-eye" onclick="togglePassword('wifiPassword', this)"></i>
        </div>
      </div>
      <h3 class="area-title"><i class="bi bi-broadcast"></i>Serveur MQTT (Mosquitto)</h3>
      <div class="input-group">
        <label><i class="bi bi-server"></i>Adresse IP du broker</label>
        <input type="text" id="mqttServerInput" placeholder="192.168.1.2" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-signpost-split"></i>Port du broker</label>
        <input type="number" id="mqttPortInput" placeholder="1883" required>
      </div>
      <div class="input-group">
        <label><i class="bi bi-person-fill"></i>Utilisateur (optionnel)</label>
        <input type="text" id="mqttUserInput" placeholder="Ex : mon_user">
      </div>
      <div class="input-group">
        <label><i class="bi bi-lock-fill"></i>Mot de passe (optionnel)</label>
        <div class="password-group">
          <input type="password" id="mqttPasswordInput" placeholder="********">
          <i class="bi bi-eye" onclick="togglePassword('mqttPasswordInput', this)"></i>
        </div>
      </div>
      <button class="btn" type="submit" onclick="submit_reseau(event)">
        <i class="bi bi-check-circle"></i>Confirmer
      </button>`;

    Promise.all([fetch("/api/wifi").then((res) => res.json()), fetch("/api/mqtt").then((res) => res.json())])
      .then(([wifiData, mqttData]) => {
        $("wifiSSIDInput").value = wifiData.ssid || "";
        $("wifiPassword").value = wifiData.password || "";
        $("mqttServerInput").value = mqttData.server || "192.168.1.2";
        $("mqttPortInput").value = mqttData.port || 1883;
        $("mqttUserInput").value = mqttData.user || "";
        $("mqttPasswordInput").value = mqttData.password || "";
      })
      .catch((err) => console.error("Erreur WiFi/MQTT fetch :", err));
  }
}

function submit_reseau(event) {
  event.preventDefault();
  const mode = ddValue("modeNet");

  if (mode === "ap") {
    const ssid = $("apSSIDInput").value.trim();
    const password = $("apPassword").value.trim();
    if (ssid === "" || password === "") return showToast("Veuillez remplir tous les champs", "warning");
    if (ssid.length < 4 || ssid.length > 20) return showToast("Le nom doit contenir entre 4 et 20 caractères", "warning");
    if (password.length < 8 || password.length > 20)
      return showToast("Le mot de passe doit contenir entre 8 et 20 caractères", "warning");

    fetch("/api/config/ap", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password }),
    })
      .then((res) => res.json())
      .then((result) => showToast(result.message || "Configuration du point d'accès enregistrée", "success"))
      .catch((error) => showToast(error.message, "error"));
  } else if (mode === "online") {
    const ssid = $("wifiSSIDInput").value.trim();
    const password = $("wifiPassword").value.trim();
    const server = $("mqttServerInput").value.trim();
    const port = parseInt($("mqttPortInput").value) || 1883;
    const user = $("mqttUserInput").value.trim();
    const pass = $("mqttPasswordInput").value.trim();

    if (ssid === "" || password === "" || server === "") return showToast("Veuillez remplir tous les champs", "warning");
    if (ssid.length < 4 || ssid.length > 20)
      return showToast("Le nom Wi-Fi doit contenir entre 4 et 20 caractères", "warning");
    if (password.length < 8 || password.length > 20)
      return showToast("Le mot de passe Wi-Fi doit contenir entre 8 et 20 caractères", "warning");

    fetch("/api/wifi", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password }),
    })
      .then((resWifi) => {
        if (!resWifi.ok) throw new Error("Échec de la configuration Wi-Fi");
        return fetch("/api/mqtt", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ server, port, user, password: pass }),
        });
      })
      .then((resMqtt) => {
        if (!resMqtt.ok) throw new Error("Échec de la configuration MQTT");
        showToast("Configuration Wi-Fi et MQTT enregistrée", "success");
      })
      .catch((err) => showToast(err.message, "error"));
  }
}

// =====================================================
// FIRMWARE
// =====================================================
function changeFirmwareMode() {
  const mode = ddValue("firmwareMode");
  const area = $("firmwareArea");

  if (mode === "offline") {
    area.innerHTML = `
      <h3 class="area-title"><i class="bi bi-usb-drive"></i>Mise à jour hors ligne</h3>
      <div class="input-group">
        <label><i class="bi bi-file-earmark-arrow-up"></i>Fichier firmware (.bin)</label>
        <input type="file" id="firmwareFile" accept=".bin" required>
      </div>
      <button class="btn" type="button" onclick="uploadOfflineFirmware(this)">
        <i class="bi bi-upload"></i>Téléverser le firmware
      </button>`;
  }

  if (mode === "online") {
    area.innerHTML = `
      <h3 class="area-title"><i class="bi bi-cloud-arrow-down"></i>Mise à jour en ligne</h3>
      <div class="input-group">
        <label><i class="bi bi-hdd-network"></i>Module à mettre à jour</label>
        <div class="dd" id="onlineModule" data-value="Module Master">
          <span data-value="Module Master">Module Master</span>
          <span data-value="Module Slave 1">Module Slave 1</span>
          <span data-value="Module Slave 2">Module Slave 2</span>
          <span data-value="Module Slave 3">Module Slave 3</span>
        </div>
      </div>
      <div class="input-group">
        <label><i class="bi bi-link-45deg"></i>URL du firmware</label>
        <input type="url" id="firmwareUrl" placeholder="https://example.com/firmware.bin" required>
      </div>
      <button class="btn" type="button" onclick="uploadOnlineFirmware()">
        <i class="bi bi-cloud-upload"></i>Télécharger et téléverser
      </button>`;
    Dropdown.init(area);
  }
}

function uploadOfflineFirmware(btn) {
  const file = $("firmwareFile").files[0];
  if (!file) return showToast("Veuillez sélectionner un fichier .bin", "warning");
  if (!file.name.endsWith(".bin")) return showToast("Seuls les fichiers .bin sont acceptés", "error");

  const formData = new FormData();
  formData.append("firmware", file);

  track(
    btn,
    fetch("/update", { method: "POST", body: formData })
      .then((res) => res.json())
      .then((result) => showToast(result.message || "Firmware envoyé avec succès", "success"))
      .catch((error) => showToast(error.message, "error"))
  );
}

function uploadOnlineFirmware() {
  const url = $("firmwareUrl").value.trim();
  if (url === "") return showToast("Veuillez saisir l'adresse du firmware", "warning");
  if (!url.startsWith("http://") && !url.startsWith("https://"))
    return showToast("L'adresse doit commencer par http:// ou https://", "error");

  fetch("http://localhost:3000/update-online", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ module: ddValue("onlineModule"), firmwareUrl: url }),
  })
    .then((res) => res.json())
    .then((result) => showToast(result.message || "Le téléchargement du firmware a démarré", "success"))
    .catch((error) => showToast(error.message, "error"));
}

// =====================================================
// PARAMÈTRES : sous-menu
// =====================================================
function showSettingPage(pageId, element) {
  document.querySelectorAll(".setting-page").forEach((p) => p.classList.remove("active"));
  $(pageId).classList.add("active");
  document.querySelectorAll(".settings-btn").forEach((b) => b.classList.remove("active"));
  element.classList.add("active");
  Dropdown.hideAll();
}

function togglePassword(inputId, icon) {
  const input = $(inputId);
  if (input.type === "password") {
    input.type = "text";
    icon.classList.replace("bi-eye", "bi-eye-slash");
  } else {
    input.type = "password";
    icon.classList.replace("bi-eye-slash", "bi-eye");
  }
}

// =====================================================
// LICENCE
// =====================================================
function randomPart(length) {
  const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const random = new Uint32Array(length);
  crypto.getRandomValues(random);
  return Array.from(random)
    .map((v) => chars[v % chars.length])
    .join("");
}

function generateLicense(mac, expiryMark) {
  const macClean = (mac || "00:00:00:00:00:00").replace(/:/g, "").toUpperCase();
  return `SMC-${macClean}-${expiryMark}-${randomPart(4)}`;
}

function getDeviceMac() {
  const el = $("deviceMAC");
  const value = window.deviceMAC || (el ? el.innerText : "") || "";
  return isValidMAC(value) ? value : null;
}

function expiryMarkFor(date, duration) {
  if (duration === 0) return "00000000";
  const d = new Date(date.getTime());
  d.setDate(d.getDate() + duration);
  return `${d.getFullYear()}${String(d.getMonth() + 1).padStart(2, "0")}${String(d.getDate()).padStart(2, "0")}`;
}

function updateLicenseExpiry() {
  const dateInput = $("licenseDate");
  const expiryInput = $("licenseExpiry");
  const codeInput = $("licenseCode");

  const creationDate = dateInput.dataset.iso ? new Date(dateInput.dataset.iso) : new Date();
  const duration = parseInt(ddValue("licenseDuration"));

  if (duration === 0) {
    expiryInput.value = "Illimitée";
    expiryInput.dataset.iso = "";
  } else {
    const expiryDate = new Date(creationDate.getTime());
    expiryDate.setDate(expiryDate.getDate() + duration);
    expiryInput.value = expiryDate.toLocaleString();
    expiryInput.dataset.iso = expiryDate.toISOString();
  }

  // Si un code est affiché, on le régénère avec la nouvelle expiration
  if (codeInput.value && codeInput.value.startsWith("SMC-")) {
    let mac = getDeviceMac();
    const previousMac = codeInput.value.match(/^SMC-([0-9A-F]{12})-/i);
    if (!mac && previousMac) mac = previousMac[1].match(/.{2}/g).join(":");
    if (!mac) return;
    codeInput.value = generateLicense(mac, expiryMarkFor(creationDate, duration));
  }
}

async function generateNewLicense() {
  let mac = getDeviceMac();
  if (!mac) {
    try {
      const response = await fetch("/api/status");
      const status = await response.json();
      if (response.ok && isValidMAC(status.mac)) {
        mac = status.mac;
        window.deviceMAC = mac;
        if ($("deviceMAC")) $("deviceMAC").innerText = mac;
      }
    } catch (_) {
      /* le message ci-dessous indique quoi faire */
    }
  }
  if (!mac) return showToast("Adresse MAC du module indisponible", "error");

  const now = new Date();
  $("licenseDate").value = now.toLocaleString();
  $("licenseDate").dataset.iso = now.toISOString();

  const duration = parseInt(ddValue("licenseDuration"));
  $("licenseCode").value = generateLicense(mac, expiryMarkFor(now, duration));

  updateLicenseExpiry();
  sendLicense(true);
}

function copyLicense() {
  const licence = $("licenseCode").value;
  if (!licence) return showToast("Aucune licence générée", "warning");
  navigator.clipboard.writeText(licence);
  showToast("Licence copiée", "success");
}

function checkLicenseStatus(expiryIso) {
  const banner = $("licenseAlertBanner");
  if (!banner) return;
  banner.style.display = expiryIso && new Date(expiryIso) < new Date() ? "flex" : "none";
}

function sendLicense(silent = false) {
  const licence = $("licenseCode").value;
  if (!licence) return showToast("Générez une licence d'abord", "warning");

  const payload = {
    licence: licence,
    date: $("licenseDate").dataset.iso || new Date().toISOString(),
    duration: parseInt(ddValue("licenseDuration")),
    expiry: $("licenseExpiry").dataset.iso || "",
  };

  fetch("/api/config/license", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  })
    .then(async (r) => {
      const result = await r.json();
      if (!r.ok) throw new Error(result.error || "Erreur serveur");
      return result;
    })
    .then((result) => {
      if (result.status === "ok") {
        if (silent !== true) showToast("Licence enregistrée sur le module", "success");
        checkLicenseStatus(payload.expiry);
      } else {
        showToast(result.error || "Erreur de sauvegarde", "error");
      }
    })
    .catch((error) => showToast("Erreur de connexion : " + error.message, "error"));
}

// =====================================================
// SEUIL DU CAPTEUR
// =====================================================
window.moduleThresholds = window.moduleThresholds || { local: 80 };

function loadSensorConfig() {
  fetch("/api/config/sensor")
    .then((res) => res.json())
    .then((data) => {
      const input = $("seuilInput");
      const slider = $("seuilSlider");
      if (!input || data.seuil === undefined) return;
      window.moduleThresholds["local"] = data.seuil;
      if (ddValue("seuilModuleSelect") === "local") {
        input.value = data.seuil;
        if (slider) {
          slider.min = data.seuilMin || 10;
          slider.max = data.seuilMax || 500;
          slider.value = data.seuil;
        }
        $("seuilValue").textContent = data.seuil + " cm";
      }
    })
    .catch((err) => console.error("Erreur fetch seuil :", err));
}

function onSeuilModuleChange(val) {
  const threshold = window.moduleThresholds[val] || 80;
  $("seuilInput").value = threshold;
  $("seuilSlider").value = threshold;
  $("seuilValue").textContent = threshold + " cm";
}

function syncSeuil(val, source) {
  if (source !== "input") $("seuilInput").value = val;
  if (source !== "slider") $("seuilSlider").value = val;
  $("seuilValue").textContent = val + " cm";
  window.moduleThresholds[ddValue("seuilModuleSelect") || "local"] = parseInt(val);
}
const syncSeuilSlider = (val) => syncSeuil(val, "slider");
const syncSeuilInput = (val) => syncSeuil(val, "input");

function saveSensorConfig(btn) {
  const val = parseInt($("seuilInput").value);
  if (isNaN(val) || val < 10 || val > 500) return showToast("Valeur invalide (10 à 500 cm)", "error");

  const target = ddValue("seuilModuleSelect") || "local";
  const payload = { seuil: val };
  if (target !== "local") payload.mac = target;

  track(
    btn,
    fetch("/api/config/sensor", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    })
      .then((res) => res.json())
      .then((data) => {
        if (data.status === "ok") {
          showToast("Seuil mis à jour : " + val + " cm", "success");
          window.moduleThresholds[target] = val;
        } else {
          showToast(data.error || "Erreur serveur", "error");
        }
      })
      .catch(() => showToast("Erreur de connexion", "error"))
  );
}

// =====================================================
// ÉTAT DU MODULE
// =====================================================
function updateDeviceStatus() {
  fetch("/api/status")
    .then((res) => res.json())
    .then((data) => {
      if ((!websocket || websocket.readyState !== WebSocket.OPEN) && $("passageCount")) {
        const total = normalizeCount(data.total);
        $("passageCount").innerText = total === null ? 0 : total;
      }

      if ($("deviceMAC")) $("deviceMAC").innerText = data.mac || "Inconnu";
      if (data.mac) window.deviceMAC = data.mac;

      const role = cap(data.role || "neutral");
      if ($("deviceRole")) $("deviceRole").innerText = data.moduleId ? `${data.moduleId} (${role})` : role;

      const isSlave = data.role === "slave";
      const modeText = isSlave
        ? data.connected
          ? "Connecté au Master AP"
          : "Déconnecté du Master AP"
        : data.connected
          ? "En ligne (STA + AP)"
          : "Hors ligne (AP)";
      if ($("deviceMode")) $("deviceMode").innerText = modeText;

      const dashMode = $("dashMode");
      if (dashMode) {
        dashMode.innerText = isSlave ? modeText : data.connected ? "En ligne" : "Hors ligne";
        const icon = dashMode.parentElement.nextElementSibling && dashMode.parentElement.nextElementSibling.querySelector("i");
        if (icon) {
          icon.className = data.connected ? "bi bi-wifi" : "bi bi-wifi-off";
          icon.parentElement.className = "card-icon " + (data.connected ? "green" : "red");
        }
      }

      // Bandeau de rôle
      const roleBannerText = $("roleBannerText");
      const roleBanner = $("roleBanner");
      if (roleBannerText && data.role) {
        const r = data.role.toLowerCase();
        const labels = {
          master: "Ce module est le Master",
          slave: data.connected ? "Ce module est un Slave (connecté au Master)" : "Ce module est un Slave (déconnecté du Master)",
        };
        roleBannerText.textContent = (data.moduleId ? `${data.moduleId} — ` : "") + (labels[r] || cap(data.role));
        if (roleBanner) roleBanner.className = "role-banner " + (r === "slave" && !data.connected ? "role-offline" : "role-" + r);
      }

      // Licence : remplit les champs vides
      if (data.licence && $("licenseCode") && !$("licenseCode").value) {
        $("licenseCode").value = data.licence;
        if (data.licenseDate) {
          $("licenseDate").value = new Date(data.licenseDate).toLocaleString();
          $("licenseDate").dataset.iso = data.licenseDate;
        }
        if (data.licenseDuration !== undefined) ddSet("licenseDuration", data.licenseDuration);
        if (data.licenseExpiry) {
          $("licenseExpiry").value = new Date(data.licenseExpiry).toLocaleString();
          $("licenseExpiry").dataset.iso = data.licenseExpiry;
        } else if (data.licenseDuration === 0) {
          $("licenseExpiry").value = "Illimitée";
        }
      }

      if (data.licenseExpiry) checkLicenseStatus(data.licenseExpiry);
      else if (data.licence && data.licenseDuration === 0) checkLicenseStatus("");
    })
    .catch((err) => console.error("Status Error:", err));
}

// =====================================================
// COMPTEURS PAR MODULE
// =====================================================
const ROLES = {
  master: { icon: "bi-star-fill", cls: "master", label: "Master" },
  slave: { icon: "bi-cpu-fill", cls: "slave", label: "Slave" },
};

function resetModule(mac) {
  if (!mac) return showToast("Adresse MAC manquante", "error");
  if (!confirm(`Réinitialiser le compteur du module (${mac}) ?`)) return;

  fetch("/api/module/reset", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ mac }),
  })
    .then((r) => r.json())
    .then((result) => {
      if (result.status === "ok") {
        showToast("Compteur réinitialisé", "success");
        updateModulesCount(true);
      } else {
        showToast(result.error || "Erreur de réinitialisation", "error");
      }
    })
    .catch(() => showToast("Erreur de connexion", "error"));
}

function formatModule(mac) {
  if (!mac) return showToast("Adresse MAC manquante", "error");
  if (!confirm(`Attention : formater et réinitialiser d'usine le module (${mac}) ? Toutes les configurations seront perdues.`)) return;

  fetch("/api/module/format", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ mac }),
  })
    .then((r) => r.json())
    .then((result) => {
      if (result.status === "ok") {
        showToast("Module formaté et redémarré", "success");
        updateModulesCount(true);
      } else {
        showToast(result.error || "Erreur lors du formatage", "error");
      }
    })
    .catch(() => showToast("Erreur de connexion", "error"));
}

let lastModulesSignature = "";

function updateModulesCount(force) {
  fetch("/api/modules_count")
    .then((res) => res.json())
    .then((data) => {
      const grid = $("moduleCountsGrid");
      if (!grid) return;

      const modules = data.modules || [];
      const normalizedTotal = normalizeCount(data.total);
      const total = normalizedTotal === null ? 0 : normalizedTotal;

      // On ne redessine que si les données ont changé (garde les boutons stables)
      const signature = JSON.stringify(data);
      if (!force && signature === lastModulesSignature) {
        const passageEl = $("passageCount");
        if (passageEl && (!websocket || websocket.readyState !== WebSocket.OPEN)) passageEl.innerText = total;
        return;
      }
      lastModulesSignature = signature;

      window.moduleThresholds = window.moduleThresholds || {};
      const seuilOptions = [{ value: "local", label: "Ce module (Master)" }];
      let hasSlaves = false;

      const label = $("moduleCountLabel");
      if (label) label.textContent = `${modules.length} module(s), total ${total}`;

      grid.innerHTML = "";

      modules.forEach((mod, idx) => {
        const r = ROLES[mod.role] || ROLES.slave;
        const isActive = mod.active !== false;

        if (mod.role === "master") {
          window.moduleThresholds["local"] = mod.seuil || 80;
        } else {
          hasSlaves = true;
          window.moduleThresholds[mod.mac] = mod.seuil || 80;
          seuilOptions.push({ value: mod.mac, label: mod.moduleId || "Slave", hint: mod.mac });
        }

        const mac = esc(mod.mac || "");
        const card = document.createElement("div");
        card.className = "module-count-card" + (isActive ? "" : " inactive");
        card.innerHTML = `
          <div class="mc-header">
            <span class="mc-icon is-${r.cls}"><i class="bi ${r.icon}"></i></span>
            <span class="mc-badge badge-${r.cls}">${r.label}</span>
            ${isActive ? "" : '<i class="bi bi-wifi-off mc-offline" title="Hors ligne"></i>'}
          </div>
          <div class="mc-body">
            <div class="mc-name">${esc(mod.moduleId || "Module " + (idx + 1))}</div>
            <div class="mc-count">${mod.count || 0}</div>
            <div class="mc-label">passages détectés</div>
            <div class="mc-mac"><i class="bi bi-cpu"></i>${mac || "—"}</div>
            <span class="mc-status ${isActive ? "online" : "offline"}">
              <i class="bi ${isActive ? "bi-wifi" : "bi-wifi-off"}"></i>${isActive ? "Connecté" : "Déconnecté"}
            </span>
            <div class="mc-actions">
              <button type="button" class="btn btn-sm btn-soft" onclick="resetModule('${mac}')">
                <i class="bi bi-arrow-counterclockwise"></i>Reset
              </button>
              <button type="button" class="btn btn-sm btn-soft-danger" onclick="formatModule('${mac}')">
                <i class="bi bi-trash"></i>Format
              </button>
            </div>
          </div>`;
        grid.appendChild(card);
      });

      const dd = Dropdown.get("seuilModuleSelect");
      if (dd) dd.setOptions(seuilOptions);
      const seuilGroup = $("seuilModuleGroup");
      if (seuilGroup) seuilGroup.style.display = hasSlaves ? "block" : "none";

      const totalCard = document.createElement("div");
      totalCard.className = "module-count-card total-card";
      totalCard.innerHTML = `
        <div class="mc-header">
          <span class="mc-icon is-total"><i class="bi bi-bar-chart-fill"></i></span>
          <span class="mc-badge">Total</span>
        </div>
        <div class="mc-body">
          <div class="mc-name">Total général</div>
          <div class="mc-count">${total}</div>
          <div class="mc-label">tous modules confondus</div>
          <div class="mc-mac"><i class="bi bi-people-fill"></i>${modules.length} module(s)</div>
        </div>`;
      grid.appendChild(totalCard);

      const passageEl = $("passageCount");
      if (passageEl && (!websocket || websocket.readyState !== WebSocket.OPEN)) passageEl.innerText = total;
    })
    .catch((err) => console.error("Erreur modules_count :", err));
}

// =====================================================
// INITIALISATION
// =====================================================
document.addEventListener("DOMContentLoaded", () => {
  Dropdown.init();
  initWebSocket();
  loadModuleConfig();

  fetch("/api/wifi")
    .then((res) => res.json())
    .then((wifiData) => {
      ddSet("modeNet", wifiData.ssid && wifiData.ssid.trim() !== "" ? "online" : "ap");
      changeNetworkMode();
    })
    .catch(() => changeNetworkMode());

  changeFirmwareMode();
  loadSensorConfig();

  updateDeviceStatus();
  setInterval(updateDeviceStatus, 1000);

  updateModulesCount();
  setInterval(updateModulesCount, 1500);
});