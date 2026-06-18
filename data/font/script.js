// script.js – Dashboard interactif + navbar

document.addEventListener("DOMContentLoaded", () => {
  // ============================
  // 1. NAVBAR : burger
  // ============================
  const burger = document.getElementById("burgerBtn");
  const navLinks = document.getElementById("navLinks");

  if (burger && navLinks) {
    burger.addEventListener("click", () => {
      const isOpen = burger.classList.toggle("open");
      navLinks.classList.toggle("active");
      burger.setAttribute("aria-expanded", isOpen);
    });

    navLinks.querySelectorAll("a").forEach((link) => {
      link.addEventListener("click", () => {
        if (window.innerWidth < 768 && burger.classList.contains("open")) {
          burger.classList.remove("open");
          navLinks.classList.remove("active");
          burger.setAttribute("aria-expanded", "false");
        }
      });
    });

    window.addEventListener("resize", () => {
      if (window.innerWidth >= 768) {
        if (burger.classList.contains("open")) {
          burger.classList.remove("open");
          burger.setAttribute("aria-expanded", "false");
        }
        navLinks.classList.remove("active");
      }
    });
  }

  // ============================
  // 2. DASHBOARD : compteur
  // ============================
  let count = 0;
  const countDisplay = document.getElementById("passageCount");
  const incrementBtn = document.getElementById("incrementBtn");
  const decrementBtn = document.getElementById("decrementBtn");
  const lastUpdateSpan = document.getElementById("lastUpdate");

  function updateCounter(newValue) {
    count = Math.max(0, newValue); // pas de négatif
    countDisplay.textContent = count;
    // mise à jour des stats dérivées
    updateDerivedStats(count);
    // horodatage
    const now = new Date();
    lastUpdateSpan.textContent = now.toLocaleTimeString("fr-FR", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
  }

  if (incrementBtn) {
    incrementBtn.addEventListener("click", () => updateCounter(count + 1));
  }
  if (decrementBtn) {
    decrementBtn.addEventListener("click", () => updateCounter(count - 1));
  }

  // ============================
  // 3. STATS dérivées (passages / heure, occupation)
  // ============================
  function updateDerivedStats(passages) {
    const perHour = document.getElementById("passPerHour");
    const occupancy = document.getElementById("occupancyRate");
    if (perHour) {
      // simulation : entre 0 et 12 passages/heure selon le compteur
      const simulated = Math.min(12, Math.round(passages * 0.6));
      perHour.textContent = simulated;
    }
    if (occupancy) {
      // simulation : entre 0 et 85%
      const rate = Math.min(85, Math.round(passages * 4.2));
      occupancy.textContent = rate + "%";
    }
  }

  // ============================
  // 4. MODE (hors ligne / en ligne)
  // ============================
  let isOffline = false;
  const toggleModeBtn = document.getElementById("toggleModeBtn");
  const toggleModeText = document.getElementById("toggleModeText");
  const modeDot = document.getElementById("modeDot");
  const modeLabel = document.getElementById("modeLabel");
  const modeStatusBadge = document.getElementById("modeStatusBadge");

  function updateMode(offline) {
    isOffline = offline;
    if (modeDot) {
      modeDot.classList.toggle("offline", offline);
    }
    if (modeLabel) {
      modeLabel.textContent = offline ? "Hors ligne" : "En ligne";
    }
    if (toggleModeText) {
      toggleModeText.textContent = offline
        ? "Passer en ligne"
        : "Passer hors ligne";
    }
    if (modeStatusBadge) {
      modeStatusBadge.textContent = offline ? "● Hors ligne" : "● Connecté";
      modeStatusBadge.className =
        "badge " + (offline ? "badge-warning" : "badge-secondary");
    }
    // Impact sur la batterie (simulation)
    if (offline) {
      // en hors ligne, la batterie diminue doucement
      simulateBatteryDrain();
    } else {
      // en ligne, on recharge
      setBatteryLevel(100);
    }
  }

  if (toggleModeBtn) {
    toggleModeBtn.addEventListener("click", () => {
      updateMode(!isOffline);
    });
  }

  // ============================
  // 5. BATTERIE
  // ============================
  let batteryLevel = 100;
  const batteryLevelEl = document.getElementById("batteryLevel");
  const batteryPercentageEl = document.getElementById("batteryPercentage");
  const batteryStatusEl = document.getElementById("batteryStatus");
  const batteryBadge = document.getElementById("batteryBadge");
  let drainInterval = null;

  function setBatteryLevel(level) {
    batteryLevel = Math.min(100, Math.max(0, level));
    if (batteryLevelEl) {
      batteryLevelEl.style.width = batteryLevel + "%";
      // couleur selon niveau
      if (batteryLevel > 60) {
        batteryLevelEl.style.background =
          "linear-gradient(90deg, #22c55e, #16a34a)";
      } else if (batteryLevel > 25) {
        batteryLevelEl.style.background =
          "linear-gradient(90deg, #eab308, #ca8a04)";
      } else {
        batteryLevelEl.style.background =
          "linear-gradient(90deg, #ef4444, #dc2626)";
      }
    }
    if (batteryPercentageEl) {
      batteryPercentageEl.textContent = batteryLevel + "%";
    }
    if (batteryStatusEl) {
      if (batteryLevel === 100) batteryStatusEl.textContent = "● Chargée";
      else if (batteryLevel > 60) batteryStatusEl.textContent = "● Bonne";
      else if (batteryLevel > 25) batteryStatusEl.textContent = "● Moyenne";
      else batteryStatusEl.textContent = "● Faible";
    }
    if (batteryBadge) {
      if (batteryLevel === 100) {
        batteryBadge.textContent = "✔ Pleine";
        batteryBadge.className = "badge badge-success";
      } else if (batteryLevel > 60) {
        batteryBadge.textContent = "● Bon niveau";
        batteryBadge.className = "badge badge-info";
      } else if (batteryLevel > 25) {
        batteryBadge.textContent = "⚠ Moyen";
        batteryBadge.className = "badge badge-warning";
      } else {
        batteryBadge.textContent = "⚠ Faible";
        batteryBadge.className = "badge badge-warning";
      }
    }
  }

  function simulateBatteryDrain() {
    // arrête l'ancien intervalle
    if (drainInterval) {
      clearInterval(drainInterval);
      drainInterval = null;
    }
    // ne draine que si hors ligne et batterie > 0
    if (isOffline && batteryLevel > 0) {
      drainInterval = setInterval(() => {
        if (isOffline && batteryLevel > 0) {
          const newLevel = Math.max(0, batteryLevel - 1);
          setBatteryLevel(newLevel);
          if (newLevel === 0) {
            clearInterval(drainInterval);
            drainInterval = null;
          }
        } else {
          clearInterval(drainInterval);
          drainInterval = null;
        }
      }, 600); // descend de 1% toutes les 0.6s (effet visible)
    }
  }

  // initialisation
  setBatteryLevel(100);
  updateMode(false); // démarre en ligne
  updateCounter(0);

  // nettoyage intervalle au cas où
  window.addEventListener("beforeunload", () => {
    if (drainInterval) clearInterval(drainInterval);
  });

  // ============================
  // 6. Simulation de variation batterie en ligne (petit effet)
  // ============================
  // On recharge un peu toutes les 8s si en ligne et pas pleine
  setInterval(() => {
    if (!isOffline && batteryLevel < 100) {
      setBatteryLevel(Math.min(100, batteryLevel + 2));
    }
    // si en ligne et batterie à 100, on reste
    if (!isOffline && batteryLevel === 100) {
      // on garde
    }
  }, 3000);
});
