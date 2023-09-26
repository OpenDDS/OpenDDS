/*
 * This selects all tabs labeled "Windows" if the OS is Windows.
 */

function autotab() {
  if (window.navigator.userAgent.indexOf("Win") != -1) {
    var li = document.getElementsByClassName("tab-label");
    for (const label of li) {
      if (label.textContent == "Windows") {
        label.onclick();
      }
    }
  }
}

document.addEventListener("DOMContentLoaded", autotab, false);
