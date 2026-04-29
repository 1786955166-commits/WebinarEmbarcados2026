/* ---------- Estado central da aplicação ---------- */

const state = {
    info: { firmware: "", author: "", site: "", contact: "" },
    datetime: "",
    relays: [false, false, false, false, false, false, false, false],
    rgb: { r: 0, g: 0, b: 0 },
    buzzer: { playing: false },
    currentDir: "/lfs1",
    files: [],
    storage: { total: 0, used: 0, pct: 0 },
};

const IMAGE_EXTS = ["jpg", "jpeg", "png", "bmp", "gif", "svg"];

/* ---------- Camada de API ---------- */
/* Wrapper apenas pra centralizar requests (logging, headers comuns). Hoje
 * é um passthrough, o browser reusa a conexão TCP via keep-alive,
 * minimizando consumo de net_conn contexts no Zephyr. */

function apiFetch(url, options) {
    return fetch(url, options);
}

const api = {
    getInfo:    ()           => apiFetch("/api/info").then(r => r.json()),
    getTime:    ()           => apiFetch("/api/time").then(r => r.json()),

    getRelays:  ()           => apiFetch("/api/relays").then(r => r.json()),
    setRelay:   (ch, on)     => apiFetch("/api/relays", {
                                    method: "POST",
                                    body: JSON.stringify({ ch: ch, state: on })
                                }).then(r => r.json()),

    getRgb:     ()           => apiFetch("/api/rgb").then(r => r.json()),
    setRgb:     (r, g, b)    => apiFetch("/api/rgb", {
                                    method: "POST",
                                    body: JSON.stringify({ r: r, g: g, b: b })
                                }).then(resp => resp.json()),

    getBuzzer:  ()           => apiFetch("/api/buzzer").then(r => r.json()),
    playBuzzer: (rtttl)      => apiFetch("/api/buzzer", {
                                    method: "POST",
                                    body: JSON.stringify({ rtttl: rtttl, action: "play" })
                                }).then(r => r.json()),
    stopBuzzer: ()           => apiFetch("/api/buzzer", {
                                    method: "POST",
                                    body: JSON.stringify({ action: "stop" })
                                }).then(r => r.json()),

    listFiles:  (dir)        => apiFetch("/api/files?dir=" + encodeURIComponent(dir)).then(r => r.json()),
    deleteFile: (path)       => apiFetch("/api/delete", {
                                    method: "POST",
                                    body: JSON.stringify({ path: path })
                                }),
    uploadFile: (dir, file)  => {
        const fd = new FormData();
        fd.append("dir", dir);
        fd.append("file", file);
        return apiFetch("/api/upload", { method: "POST", body: fd });
    },
    fetchText:  (url)        => apiFetch(url).then(r => r.ok ? r.text() : Promise.reject(r.status)),
};

/* ---------- Helpers ---------- */

function rgbToHex(r, g, b) {
    return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
}

function formatSize(bytes) {
    if (bytes < 1024)    return bytes + " B";
    if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB";
    return (bytes / 1048576).toFixed(1) + " MB";
}

function getExt(name) {
    const dot = name.lastIndexOf(".");
    return dot >= 0 ? name.substring(dot + 1).toLowerCase() : "";
}

function $(id) { return document.getElementById(id); }

/* ---------- Render: lê de `state` e atualiza o DOM ---------- */

const render = {
    info() {
        const { firmware, author, site, contact } = state.info;
        $("info-firmware").textContent = firmware || "-";
        $("info-author").textContent = author || "-";
        const siteEl = $("info-site");
        siteEl.textContent = site || "-";
        siteEl.href = site ? "https://" + site : "#";
        const contactEl = $("info-contact");
        contactEl.textContent = contact || "-";
        contactEl.href = contact ? "mailto:" + contact : "#";
    },

    datetime() {
        $("datetime-display").textContent = state.datetime || "--";
    },

    storage() {
        const { total, used, pct } = state.storage;
        $("storage-total").textContent = total ? formatSize(total) : "-";
        $("storage-used").textContent = total ? `${formatSize(used)} (${pct}%)` : "-";
    },

    relays() {
        const grid = $("relay-grid");
        let html = "";
        for (let i = 0; i < 8; i++) {
            const ch = i + 1;
            const on = state.relays[i];
            const cls = on ? "relay-on" : "relay-off";
            const label = on ? "ON" : "OFF";
            const btn = on ? "Turn OFF" : "Turn ON";
            html += `<div class="relay-item ${cls}" id="relay-${ch}">`;
            html +=   `<div class="relay-label">CH${ch}</div>`;
            html +=   `<div><span class="relay-dot"></span>${label}</div>`;
            html +=   `<button class="relay-btn" onclick="toggleRelay(${ch})">${btn}</button>`;
            html += `</div>`;
        }
        grid.innerHTML = html;
    },

    rgb() {
        const { r, g, b } = state.rgb;
        $("rgb-r").value = r;
        $("rgb-g").value = g;
        $("rgb-b").value = b;
        $("rgb-r-val").textContent = r;
        $("rgb-g-val").textContent = g;
        $("rgb-b-val").textContent = b;
        $("rgb-preview").style.background = `rgb(${r},${g},${b})`;
        $("rgb-colorpicker").value = rgbToHex(r, g, b);
    },

    buzzerStatus(text) {
        $("buzzer-status").textContent = text != null ? text :
            (state.buzzer.playing ? "Playing" : "");
    },

    breadcrumb() {
        const bc = $("breadcrumb");
        const parts = state.currentDir.split("/").filter(p => p.length > 0);
        let html = `<a href="#" onclick="navigateTo('/lfs1'); return false;">/lfs1</a>`;
        let path = "/lfs1";
        for (let i = 1; i < parts.length; i++) {
            path += "/" + parts[i];
            html += ` / <a href="#" onclick="navigateTo('${path}'); return false;">${parts[i]}</a>`;
        }
        bc.innerHTML = html;
    },

    files() {
        const tbody = $("file-list");
        render.breadcrumb();

        if (!state.files || state.files.length === 0) {
            tbody.innerHTML = "<tr><td colspan='3'>Empty directory</td></tr>";
            return;
        }

        let html = "";
        if (state.currentDir !== "/lfs1") {
            let parent = state.currentDir.substring(0, state.currentDir.lastIndexOf("/"));
            if (!parent || parent.length < 5) parent = "/lfs1";
            html += `<tr><td><a class="dir-link" href="#" onclick="navigateTo('${parent}'); return false;">..</a></td><td></td><td></td></tr>`;
        }

        const subPath = state.currentDir.substring(5);
        for (const f of state.files) {
            if (f.type === "dir") {
                const dirPath = state.currentDir + "/" + f.name;
                html += `<tr><td><a class="dir-link" href="#" onclick="navigateTo('${dirPath}'); return false;">${f.name}/</a></td>`;
                html += `<td>-</td><td></td></tr>`;
            } else {
                const dlPath = "/download" + (subPath ? subPath : "") + "/" + f.name;
                const filePath = state.currentDir + "/" + f.name;
                html += `<tr><td>${f.name}</td>`;
                html += `<td>${formatSize(f.size)}</td>`;
                html += `<td><button class="btn-view" onclick="viewFile('${dlPath}','${f.name}')">View</button> `;
                html += `<a href="${dlPath}" download>Download</a> `;
                html += `<button class="btn-del" onclick="deleteFile('${filePath}')">Delete</button></td></tr>`;
            }
        }
        tbody.innerHTML = html;
    },

    uploadStatus(text) { $("status").textContent = text; },
};

/* ---------- Controllers (event handlers) ---------- */
/* Padrão: chama api → atualiza state → render. */

async function fetchInfo() {
    try {
        const data = await api.getInfo();
        state.info = data;
        render.info();
    } catch (e) {
        console.error("Info fetch error:", e);
    }
}

async function fetchTime() {
    try {
        const data = await api.getTime();
        state.datetime = data.datetime;
        render.datetime();
    } catch (e) {
        console.error("Time fetch error:", e);
    }
}

async function fetchRelays() {
    try {
        const data = await api.getRelays();
        state.relays = data.relays;
        render.relays();
    } catch (e) {
        console.error("Relay fetch error:", e);
    }
}

async function toggleRelay(ch) {
    const newState = !state.relays[ch - 1];
    try {
        const data = await api.setRelay(ch, newState);
        state.relays = data.relays;
        render.relays();
    } catch (e) {
        console.error("Relay toggle error:", e);
    }
}

/* RGB */

function readRgbFromInputs() {
    return {
        r: parseInt($("rgb-r").value, 10),
        g: parseInt($("rgb-g").value, 10),
        b: parseInt($("rgb-b").value, 10),
    };
}

function onRgbSlider() {
    state.rgb = readRgbFromInputs();
    render.rgb();
}

function onColorPicker() {
    const hex = $("rgb-colorpicker").value;
    state.rgb = {
        r: parseInt(hex.substring(1, 3), 16),
        g: parseInt(hex.substring(3, 5), 16),
        b: parseInt(hex.substring(5, 7), 16),
    };
    render.rgb();
}

async function fetchRgb() {
    try {
        const data = await api.getRgb();
        state.rgb = { r: data.r, g: data.g, b: data.b };
        render.rgb();
    } catch (e) {
        console.error("RGB fetch error:", e);
    }
}

async function sendRgb() {
    const { r, g, b } = readRgbFromInputs();
    try {
        const data = await api.setRgb(r, g, b);
        state.rgb = { r: data.r, g: data.g, b: data.b };
        render.rgb();
    } catch (e) {
        console.error("RGB send error:", e);
    }
}

async function sendRgbOff() {
    try {
        await api.setRgb(0, 0, 0);
        state.rgb = { r: 0, g: 0, b: 0 };
        render.rgb();
    } catch (e) {
        console.error("RGB off error:", e);
    }
}

/* Buzzer */

function buzzerPreset() {
    const sel = $("buzzer-presets");
    if (sel.value) {
        $("buzzer-rtttl").value = sel.value;
    }
    sel.selectedIndex = 0;
}

async function buzzerPlay() {
    const rtttl = $("buzzer-rtttl").value.trim();

    if (!rtttl) {
        alert("Enter an RTTTL string or select a preset.");
        return;
    }

    const parts = rtttl.split(":");
    if (parts.length < 3 || parts[0].length === 0 || parts[2].trim().length === 0) {
        alert("Invalid RTTTL format.\nExpected: Name:d=N,o=N,b=N:note1,note2,...");
        return;
    }

    render.buzzerStatus("Sending...");
    try {
        const data = await api.playBuzzer(rtttl);
        if (data.ok) {
            state.buzzer.playing = true;
            render.buzzerStatus("Playing");
        } else {
            render.buzzerStatus("");
            alert("Buzzer error: " + (data.error || "Unknown error"));
        }
    } catch (e) {
        render.buzzerStatus("");
        alert("Buzzer request failed: " + e.message);
    }
}

async function buzzerStop() {
    try {
        const data = await api.stopBuzzer();
        state.buzzer.playing = !!data.playing;
        render.buzzerStatus(state.buzzer.playing ? "Playing" : "Stopped");
    } catch (e) {
        console.error("Buzzer stop error:", e);
    }
}

async function fetchBuzzerStatus() {
    try {
        const data = await api.getBuzzer();
        state.buzzer.playing = !!data.playing;
        render.buzzerStatus();
    } catch (e) {
        console.error("Buzzer status error:", e);
    }
}

/* Files */

function navigateTo(dir) {
    state.currentDir = dir;
    fetchFiles();
}

async function fetchFiles() {
    const tbody = $("file-list");
    tbody.innerHTML = "<tr><td colspan='3'>Loading...</td></tr>";
    render.breadcrumb();

    try {
        const data = await api.listFiles(state.currentDir);
        state.files = data.files || [];
        state.storage = {
            total: data.total_bytes || 0,
            used:  data.used_bytes  || 0,
            pct:   data.used_pct    || 0,
        };
        render.files();
        render.storage();
    } catch (e) {
        tbody.innerHTML = `<tr><td colspan='3'>Error: ${e.message || e}</td></tr>`;
    }
}

async function uploadFile() {
    const input = $("file-input");
    if (!input.files || input.files.length === 0) {
        render.uploadStatus("Select a file first.");
        return;
    }

    const file = input.files[0];
    render.uploadStatus("Uploading " + file.name + "...");

    try {
        const r = await api.uploadFile(state.currentDir, file);
        if (r.ok) {
            render.uploadStatus("Upload complete: " + file.name);
            input.value = "";
            fetchFiles();
        } else {
            render.uploadStatus("Upload failed: " + r.status);
        }
    } catch (e) {
        render.uploadStatus("Upload error: " + e.message);
    }
}

async function deleteFile(path) {
    if (!confirm("Delete " + path.split("/").pop() + "?")) return;
    try {
        const r = await api.deleteFile(path);
        if (r.ok) {
            fetchFiles();
        } else {
            alert("Delete failed: " + r.status);
        }
    } catch (e) {
        alert("Delete error: " + e.message);
    }
}

async function viewFile(dlPath, name) {
    const modal = $("modal");
    const body = $("modal-body");
    $("modal-title").textContent = name;
    body.innerHTML = "<p>Loading...</p>";
    modal.classList.add("active");

    const ext = getExt(name);
    try {
        if (IMAGE_EXTS.indexOf(ext) >= 0) {
            body.innerHTML = `<img src="${dlPath}" alt="${name}">`;
        } else {
            const text = await api.fetchText(dlPath);
            const pre = document.createElement("pre");
            pre.textContent = text;
            body.innerHTML = "";
            body.appendChild(pre);
        }
    } catch (e) {
        body.innerHTML = `<p>Error: ${e.message || e}</p>`;
    }
}

function closeModal(event) {
    if (event && event.target !== $("modal")) return;
    $("modal").classList.remove("active");
    $("modal-body").innerHTML = "";
}

/* ---------- Init ---------- */

const TIME_POLL_INTERVAL_MS = 1000;

/* Sequential init, keeps only one TCP connection in flight at a time
 * to avoid exhausting CONFIG_NET_MAX_CONN on the Zephyr side. */
window.addEventListener("DOMContentLoaded", async () => {
    await fetchInfo();
    await fetchTime();
    await fetchRelays();
    await fetchRgb();
    await fetchBuzzerStatus();
    await fetchFiles();
    setInterval(fetchTime, TIME_POLL_INTERVAL_MS);
});
