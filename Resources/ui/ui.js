(function() {
    'use strict';

    const controls = {};
    let waveformData = [];
    let currentPresetIndex = -1;

    function init() {
        bindControls();
        setupTransport();
        setupSliders();
        setupKnobs();
        setupAbout();
        setupPresets();
        setupWaveform();
        setupEventListeners();
        notifyReady();
    }

     function bindControls() {
         controls.kickBtn = document.getElementById('kick-btn');
         controls.presetSelect = document.getElementById('preset-select');
         controls.saveBtn = document.getElementById('save-btn');
         controls.loadBtn = document.getElementById('load-btn');
         controls.aboutBtn = document.getElementById('about-btn');
         controls.aboutOverlay = document.getElementById('about-overlay');
         controls.closeAbout = document.getElementById('close-about');
         controls.previewCanvas = document.getElementById('preview-canvas');
         controls.meterFill = document.getElementById('meter-fill');
         controls.meterVal = document.getElementById('meter-val');
         controls.clipIndicator = document.getElementById('clip-indicator');
         controls.presetPrev = document.getElementById('preset-prev');
         controls.presetNext = document.getElementById('preset-next');
         controls.abToggle = document.getElementById('ab-toggle');
         controls.limiterToggle = document.getElementById('limiter-toggle');
         controls.loopToggle = document.getElementById('loop-toggle');
         controls.bpmDisplay = document.getElementById('bpm-display');
         controls.bpmDown = document.getElementById('bpm-down');
         controls.bpmUp = document.getElementById('bpm-up');
         controls.oversampleSelect = document.getElementById('oversample-select');
     }

    function setupPresets() {
        controls.presetSelect.addEventListener('change', function() {
            const raw = this.value;
            if (raw === '' || raw === '-1') return;
            const val = parseInt(raw, 10);
            if (!isNaN(val) && val >= 0) {
                currentPresetIndex = val;
                emitEvent('parameterChange', { param: 'preset', value: val });
            }
        });

        controls.saveBtn.addEventListener('click', () => emitEvent('savePreset', null));
        controls.loadBtn.addEventListener('click', () => emitEvent('loadPreset', null));

        controls.presetPrev.addEventListener('click', () => {
            if (currentPresetIndex > 0) {
                currentPresetIndex--;
                controls.presetSelect.value = currentPresetIndex;
                emitEvent('parameterChange', { param: 'preset', value: currentPresetIndex });
            }
        });

        controls.presetNext.addEventListener('click', () => {
            if (currentPresetIndex < 12) {
                currentPresetIndex++;
                controls.presetSelect.value = currentPresetIndex;
                emitEvent('parameterChange', { param: 'preset', value: currentPresetIndex });
            }
        });

        controls.abToggle.addEventListener('click', function() {
            this.classList.toggle('active');
        });
    }

     function setupTransport() {
         controls.kickBtn.addEventListener('mousedown', () => emitEvent('trigger', 1));

         const waveOptions = document.querySelectorAll('#waveform .wave-btn');
         waveOptions.forEach(function(opt) {
             opt.addEventListener('click', function() {
                 waveOptions.forEach(o => {
                     o.classList.remove('active');
                     o.setAttribute('aria-checked', 'false');
                 });
                 this.classList.add('active');
                 this.setAttribute('aria-checked', 'true');
                 emitEvent('parameterChange', { param: 'waveform', value: parseInt(this.dataset.val, 10) });
             });
         });

         const driveBtns = document.querySelectorAll('.drive-type-selector .drive-btn');
         driveBtns.forEach(function(btn) {
             btn.addEventListener('click', function() {
                 driveBtns.forEach(b => b.classList.remove('active'));
                 this.classList.add('active');
                 emitEvent('parameterChange', { param: 'driveType', value: this.dataset.val });
             });
         });

         const envBtns = document.querySelectorAll('.env-toggle .env-btn');
         envBtns.forEach(function(btn) {
             btn.addEventListener('click', function() {
                 envBtns.forEach(b => b.classList.remove('active'));
                 this.classList.add('active');
             });
         });

        controls.limiterToggle.addEventListener('click', function() {
            const active = !this.classList.contains('active');
            this.classList.toggle('active');
            this.setAttribute('aria-pressed', active);
            emitEvent('parameterChange', { param: 'limiter', value: active ? 1 : 0 });
        });

        controls.loopToggle.addEventListener('click', function() {
            const active = !this.classList.contains('active');
            this.classList.toggle('active');
            this.setAttribute('aria-pressed', active);
            emitEvent('parameterChange', { param: 'loopEnabled', value: active ? 1 : 0 });
        });

         controls.bpmDown.addEventListener('click', () => {
             const cur = parseInt(controls.bpmDisplay.textContent || '120', 10);
             const newVal = Math.max(80, cur - 1);
             controls.bpmDisplay.textContent = newVal;
             emitEvent('parameterChange', { param: 'bpm', value: newVal });
         });

         controls.bpmUp.addEventListener('click', () => {
             const cur = parseInt(controls.bpmDisplay.textContent || '120', 10);
             const newVal = Math.min(250, cur + 1);
             controls.bpmDisplay.textContent = newVal;
             emitEvent('parameterChange', { param: 'bpm', value: newVal });
         });

         controls.oversampleSelect.addEventListener('change', function() {
             const val = parseInt(this.value, 10);
             emitEvent('parameterChange', { param: 'oversample', value: val });
         });
     }

    function setupAbout() {
        controls.aboutBtn.addEventListener('click', () => {
            if (controls.aboutOverlay) controls.aboutOverlay.classList.remove('hidden');
        });
        if (controls.closeAbout) {
            controls.closeAbout.addEventListener('click', () => {
                if (controls.aboutOverlay) controls.aboutOverlay.classList.add('hidden');
            });
        }
        if (controls.aboutOverlay) {
            controls.aboutOverlay.addEventListener('click', (e) => {
                if (e.target === controls.aboutOverlay) controls.aboutOverlay.classList.add('hidden');
            });
        }
    }

    function setupEventListeners() {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.addEventListener('initUI', function(data) {
                window.updateUI(data);
            });
            window.__JUCE__.backend.addEventListener('meterUpdate', function(data) {
                window.updateMeter(data);
            });
            window.__JUCE__.backend.addEventListener('waveformUpdate', function(data) {
                window.updateWaveform(data);
            });
            window.__JUCE__.backend.addEventListener('blinkKick', function() {
                window.blinkKick();
            });
        } else {
            console.warn('JUCE backend not available');
        }
    }

    function notifyReady() {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.emitEvent('uiReady', null);
        }
    }

    function emitEvent(type, data) {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.emitEvent(type, data);
        }
    }

    const activeKnobs = {};

    function setupSliders() {
        setupParamSlider('slider-amp-sustain', 'ampSustain', formatSustainPct);
        setupParamSlider('slider-amp-release', 'ampRelease', formatAdsrTime);
    }

    function setupParamSlider(sliderId, param, formatFn) {
        const input = document.getElementById(sliderId);
        if (!input) return;

        const dispId = 'val-' + sliderId.replace('slider-', '').replace('amp-', 'amp-');
        const disp = document.getElementById(dispId);
        const min = parseFloat(input.min);
        const max = parseFloat(input.max);

        const syncFromValue = (val) => {
            const v = Math.max(min, Math.min(max, typeof val === 'number' ? val : parseFloat(val)));
            input.value = String(v);
            if (disp) disp.textContent = formatFn(v);
        };

        input.addEventListener('input', () => {
            const v = parseFloat(input.value);
            if (disp) disp.textContent = formatFn(v);
            emitEvent('parameterChange', { param: param, value: v });
        });

        activeKnobs[param] = syncFromValue;
    }

    /* ── SVG Arc Helpers ── */
    function describeArc(cx, cy, r, startDeg, endDeg) {
        const startRad = startDeg * Math.PI / 180;
        const endRad = endDeg * Math.PI / 180;
        const x1 = cx + r * Math.cos(startRad);
        const y1 = cy + r * Math.sin(startRad);
        const x2 = cx + r * Math.cos(endRad);
        const y2 = cy + r * Math.sin(endRad);
        const large = (endDeg - startDeg) > 180 ? 1 : 0;
        const sweep = 1;
        return 'M' + x1 + ' ' + y1 + ' A' + r + ' ' + r + ' 0 ' + large + ' ' + sweep + ' ' + x2 + ' ' + y2;
    }

    function injectKnobTrack(container, isSmall) {
        const size = isSmall ? 34 : 42;
        const cx = size / 2;
        const r = isSmall ? 12 : 16;
        const strokeW = isSmall ? 1.5 : 2.5;

        const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        svg.setAttribute('class', 'knob-svg-track');
        svg.setAttribute('viewBox', '0 0 ' + size + ' ' + size);
        svg.setAttribute('width', String(size));
        svg.setAttribute('height', String(size));

        const startAngle = 130;
        const endAngle = 410;

        const bgPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        bgPath.setAttribute('class', 'knob-track-bg');
        bgPath.setAttribute('d', describeArc(cx, cx, r, startAngle, endAngle));
        bgPath.setAttribute('stroke-width', String(strokeW));

        const fillPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        fillPath.setAttribute('class', 'knob-track-fill');
        fillPath.setAttribute('d', describeArc(cx, cx, r, startAngle, startAngle));
        fillPath.setAttribute('stroke-width', String(strokeW));

        svg.appendChild(bgPath);
        svg.appendChild(fillPath);
        container.appendChild(svg);
        return { svg, fillPath, startAngle, cx, r };
    }

    function updateKnobArc(fillPath, startAngle, cx, r, percentage) {
        const endAngle = startAngle + percentage * 280;
        fillPath.setAttribute('d', describeArc(cx, cx, r, startAngle, endAngle));
    }

     function setupKnobs() {
         setupKnob('knob-pitch', 40, 250, 'pitch', (v) => Math.round(v) + ' Hz', 196);
         setupKnob('knob-fine', -100, 100, 'fine', (v) => v.toFixed(0) + ' ct', 0);
         setupKnob('knob-pitch-decay', 0.05, 0.5, 'pitchDecay', (v) => v.toFixed(2), 0.23);
         setupKnob('knob-pitch-curve', 0, 1, 'pitchCurve', (v) => v.toFixed(2), 0.50);
         setupKnob('knob-amp-attack', 0, 0.05, 'ampAttack', formatAdsrTime, 0.001);
         setupKnob('knob-click', 0, 1, 'click', (v) => v.toFixed(2), 0);
         setupKnob('knob-amp-decay', 0.05, 1, 'ampDecay', formatAdsrTime, 0.30);
         setupKnob('knob-click-pitch', 500, 8000, 'clickPitch', (v) => Math.round(v), 4000);
         setupKnob('knob-depth', 0, 1, 'depth', (v) => v.toFixed(2), 0.50);
         setupKnob('knob-drive', 0.5, 3, 'drive', (v) => v.toFixed(2), 1.0);
         setupKnob('knob-color', 0, 1, 'color', (v) => v.toFixed(2), 0.50);
         setupKnob('knob-wave-morph', 0, 1, 'waveMorph', (v) => v.toFixed(2), 0);
         setupKnob('knob-hp', 25, 600, 'hpCutoff', (v) => Math.round(v) + ' Hz', 35);
         setupKnob('knob-lp', 800, 20000, 'lpCutoff', formatLpHz, 20000);
         setupKnob('knob-gain', 0, 1.5, 'gain', (v) => v.toFixed(2), 1.0);
         setupKnob('knob-enhancer', 0, 1, 'enhancer', (v) => v.toFixed(2), 0);

         setupKeyNoteControls();
     }

    function setupKnob(id, min, max, param, formatFn, defaultValue) {
        const container = document.getElementById(id);
        if (!container) return;
        const dial = container.querySelector('.knob-dial');
        const displayId = 'val-' + id.replace('knob-', '');
        const display = document.getElementById(displayId);
        const isSmall = container.classList.contains('small');
        let currentVal = min;

        const arc = injectKnobTrack(container, isSmall);

        const updateVisuals = (val) => {
            currentVal = Math.max(min, Math.min(max, val));
            const percentage = (currentVal - min) / (max - min);
            const angle = -140 + (percentage * 280);
            if (dial) dial.style.transform = 'rotate(' + angle + 'deg)';
            if (display) display.textContent = formatFn(currentVal);
            updateKnobArc(arc.fillPath, arc.startAngle, arc.cx, arc.r, percentage);
        };

        let isDragging = false;
        let startY = 0;
        let startVal = 0;
        let lastVal = 0;
        let lastEmit = 0;

        container.addEventListener('mousedown', (e) => {
            e.preventDefault();
            e.stopPropagation();
            isDragging = true;
            startY = e.clientY;
            startVal = currentVal;
            lastVal = currentVal;
            lastEmit = performance.now();
            document.body.style.cursor = 'ns-resize';
        });

        window.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            const deltaY = startY - e.clientY;
            const shift = e.shiftKey ? 0.1 : 1;
            const range = (max - min);
            const sensitivity = shift ? range / 1200 : range / 120;
            const deltaVal = deltaY * sensitivity;
            const newVal = Math.max(min, Math.min(max, startVal + deltaVal));
            updateVisuals(newVal);
            const now = performance.now();
            if (newVal !== lastVal || now - lastEmit > 50) {
                lastVal = newVal;
                lastEmit = now;
                emitEvent('parameterChange', { param: param, value: newVal });
            }
        }, { passive: false });

        window.addEventListener('mouseup', () => {
            if (isDragging) {
                if (lastVal !== startVal || Math.abs(currentVal - startVal) > 0) {
                    emitEvent('parameterChange', { param: param, value: currentVal });
                }
                isDragging = false;
                document.body.style.cursor = 'default';
            }
        });

        container.addEventListener('dblclick', (e) => {
            e.preventDefault();
            e.stopPropagation();
            if (defaultValue !== undefined) {
                updateVisuals(defaultValue);
                emitEvent('parameterChange', { param: param, value: defaultValue });
            }
        });

        container.addEventListener('wheel', (e) => {
            e.preventDefault();
            const range = (max - min);
            const step = range * 0.02;
            const newVal = Math.max(min, Math.min(max, currentVal + (e.deltaY < 0 ? step : -step)));
            updateVisuals(newVal);
            emitEvent('parameterChange', { param: param, value: newVal });
        }, { passive: false });

        activeKnobs[param] = updateVisuals;
    }

    function setupKeyNoteControls() {
        const rail = document.querySelector('.key-row');
        const octDown = document.getElementById('key-oct-down');
        const octUp = document.getElementById('key-oct-up');
        const octDisplay = document.getElementById('key-oct-display');
        if (!rail || !octDown || !octUp || !octDisplay) return;

        function setOctLabel(val) {
            const v = Math.max(0, Math.min(5, Math.round(Number(val))));
            octDisplay.textContent = v;
            octDown.disabled = v <= 0;
            octUp.disabled = v >= 5;
        }

        function setActiveNote(val) {
            const n = Math.max(0, Math.min(11, Math.round(Number(val))));
            rail.querySelectorAll('.key-note').forEach(function(btn) {
                const on = parseInt(btn.dataset.note, 10) === n;
                btn.classList.toggle('key-active', on);
            });
        }

        activeKnobs.keyNote = setActiveNote;
        activeKnobs.keyOctave = setOctLabel;

        rail.querySelectorAll('.key-note').forEach(function(btn) {
            btn.addEventListener('click', function() {
                emitEvent('parameterChange', { param: 'keyNote', value: parseInt(btn.dataset.note, 10) });
            });
        });

        octDown.addEventListener('click', () => {
            const cur = parseInt(octDisplay.textContent || '3', 10);
            emitEvent('parameterChange', { param: 'keyOctave', value: Math.max(0, cur - 1) });
        });
        octUp.addEventListener('click', () => {
            const cur = parseInt(octDisplay.textContent || '3', 10);
            emitEvent('parameterChange', { param: 'keyOctave', value: Math.min(5, cur + 1) });
        });
    }

    function formatAdsrTime(seconds) {
        const s = Math.max(0, seconds);
        if (s < 1) {
            const ms = s * 1000;
            return ms < 10 ? ms.toFixed(1) + ' ms' : Math.round(ms) + ' ms';
        }
        return s.toFixed(2) + ' s';
    }

    function formatSustainPct(v) {
        return Math.round(Math.max(0, Math.min(1, v)) * 100) + '%';
    }

    function formatLpHz(v) {
        const x = Math.max(800, Math.min(20000, Number(v)));
        if (x >= 1000) {
            const k = x / 1000;
            return (Math.abs(k - Math.round(k)) < 0.06 ? String(Math.round(k)) : k.toFixed(1)) + ' kHz';
        }
        return Math.round(x) + ' Hz';
    }

    function setupWaveform() {
        const canvas = controls.previewCanvas;
        if (!canvas) return;
        const parent = canvas.parentElement;
        const w = parent ? parent.clientWidth : 410;
        const h = parent ? parent.clientHeight : 120;
        canvas.width = Math.max(100, w);
        canvas.height = Math.max(40, h);
        waveformData = new Array(450).fill(0);
        drawWaveform();
    }

    function drawWaveform() {
        const canvas = controls.previewCanvas;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        const width = canvas.width;
        const height = canvas.height;
        const centerY = height / 2;

        ctx.clearRect(0, 0, width, height);

        const gradient = ctx.createLinearGradient(0, 0, 0, height);
        gradient.addColorStop(0, '#2a1a31');
        gradient.addColorStop(0.5, '#392443');
        gradient.addColorStop(1, '#2a1a31');
        ctx.fillStyle = gradient;
        ctx.fillRect(0, 0, width, height);

        const gridColor = 'rgba(74,50,85,0.4)';
        ctx.strokeStyle = gridColor;
        ctx.lineWidth = 0.5;
        for (let i = 0; i <= 4; i++) {
            const y = (height / 4) * i;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(width, y);
            ctx.stroke();
        }
        for (let i = 0; i <= 8; i++) {
            const x = (width / 8) * i;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, height);
            ctx.stroke();
        }

        ctx.shadowColor = 'rgba(0,160,176,0.3)';
        ctx.shadowBlur = 6;
        ctx.strokeStyle = '#00a0b0';
        ctx.lineWidth = 1.5;
        ctx.beginPath();

        for (let i = 0; i < width; i++) {
            const dataIndex = Math.floor(i * waveformData.length / width);
            const sample = waveformData[dataIndex] || 0;
            const y = centerY - sample * centerY * 0.9;
            if (i === 0) { ctx.moveTo(i, y); } else { ctx.lineTo(i, y); }
        }
        ctx.stroke();

        ctx.shadowBlur = 0;
        ctx.strokeStyle = 'rgba(0,160,176,0.2)';
        ctx.lineWidth = 0.5;
        ctx.beginPath();
        for (let j = 0; j < width; j++) {
            const di = Math.floor(j * waveformData.length / width);
            const s = waveformData[di] || 0;
            const y2 = centerY + s * centerY * 0.9;
            if (j === 0) { ctx.moveTo(j, y2); } else { ctx.lineTo(j, y2); }
        }
        ctx.stroke();
    }

     window.updateUI = function(data) {
         if (controls.presetSelect && data.factoryPreset !== undefined) {
             const fp = data.factoryPreset;
             if (typeof fp === 'number' && fp >= 0) {
                 controls.presetSelect.value = String(Math.floor(fp));
                 currentPresetIndex = Math.floor(fp);
             }
         }

         if (data.waveform !== undefined) {
             const waveBtns = document.querySelectorAll('#waveform .wave-btn');
             waveBtns.forEach(function(o) {
                 const on = parseInt(o.dataset.val, 10) === data.waveform;
                 o.classList.toggle('active', on);
                 o.setAttribute('aria-checked', on ? 'true' : 'false');
             });
         }

          if (data.limiter !== undefined) {
            const isActive = !!data.limiter;
            controls.limiterToggle.classList.toggle('active', isActive);
            controls.limiterToggle.setAttribute('aria-pressed', isActive);
        }

          if (data.loopEnabled !== undefined) {
            const isActive = !!data.loopEnabled;
            controls.loopToggle.classList.toggle('active', isActive);
            controls.loopToggle.setAttribute('aria-pressed', isActive);
        }

         if (data.oversample !== undefined) {
             controls.oversampleSelect.value = String(Math.floor(data.oversample));
         }

         if (data.bpm !== undefined) {
             controls.bpmDisplay.textContent = Math.round(data.bpm);
         }

         Object.keys(activeKnobs).forEach(function(param) {
             if (data[param] !== undefined) {
                 activeKnobs[param](data[param]);
             }
         });
     };

    window.blinkKick = function() {
        if (!controls.kickBtn) return;
        controls.kickBtn.style.transform = 'scale(0.92)';
        controls.kickBtn.style.opacity = '0.7';
        setTimeout(() => {
            controls.kickBtn.style.transform = '';
            controls.kickBtn.style.opacity = '';
        }, 100);
    };

    window.updateWaveform = function(data) {
        waveformData = data;
        drawWaveform();
    };

    window.updateMeter = function(db) {
        const meterFill = controls.meterFill;
        const meterLabel = controls.meterVal;
        const clipIndicator = controls.clipIndicator;
        if (!meterFill || !meterLabel) return;

        const dbValue = Math.max(-60, db);
        const percentage = ((dbValue + 60) / 60) * 100;

        requestAnimationFrame(function() {
            meterFill.style.width = Math.max(0, Math.min(100, percentage)) + '%';

            const clamped = Math.max(-60, Math.min(0, dbValue));
            const hue = 120 - (clamped + 60) * 2;
            meterFill.style.background = 'linear-gradient(90deg, #2ED573, #FFD93D 50%, #FF8A47 75%, #FF4757)';

            if (clamped >= -0.5) {
                meterLabel.classList.add('clip');
            } else {
                meterLabel.classList.remove('clip');
            }

            meterLabel.textContent = dbValue > -60 ? dbValue.toFixed(1) + ' dB' : '-\u221E dB';

            if (clipIndicator) {
                if (dbValue >= -0.5) {
                    clipIndicator.classList.add('visible');
                    setTimeout(() => clipIndicator.classList.remove('visible'), 500);
                }
            }
        });
    };

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
