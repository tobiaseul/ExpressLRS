import {html, LitElement} from "lit"
import {customElement, state} from "lit/decorators.js"
import '../assets/mui.js'
import {_renderOptions} from "../utils/libs.js"
import {elrsState, saveOptionsAndConfig} from "../utils/state.js"
import {postJSON, infoAlert, errorAlert} from "../utils/feedback.js"

@customElement('led-config-panel')
class LedConfigPanel extends LitElement {
    @state() accessor selectedPreset = 0
    @state() accessor selectedStrip = 0
    @state() accessor selectedPixel = 0
    @state() accessor auxChannelPreset = 255
    @state() accessor auxChannelBright = 255
    @state() accessor currentBrightness = 100
    @state() accessor ledConfig = null

    createRenderRoot() {
        this.loadConfig()
        this.save = this.save.bind(this)
        return this
    }

    loadConfig() {
        // Load LED config from global state
        if (elrsState.config.ledConfig) {
            this.ledConfig = structuredClone(elrsState.config.ledConfig)
            this.selectedPreset = this.ledConfig.currentPreset || 0
            this.currentBrightness = this.ledConfig.currentBrightness || 100
            this.auxChannelPreset = this.ledConfig.auxChannelPreset || 255
            this.auxChannelBright = this.ledConfig.auxChannelBright || 255
        }
    }

    render() {
        if (!this.ledConfig) {
            return html`<div class="mui-panel">LED configuration not available (ESP32 only)</div>`
        }

        const preset = this.ledConfig.presets[this.selectedPreset]
        const strip = preset ? preset.strips[this.selectedStrip] : null
        const pixel = strip ? strip.pixels[this.selectedPixel] : null

        return html`
            <div class="mui-panel mui--text-title">LED Configuration</div>
            <div class="mui-panel">
                <h2>TX Control Setup</h2>
                <p>Configure which AUX channels control LED presets and brightness during flight.</p>

                <div class="mui-select">
                    <select @change="${(e) => this.auxChannelPreset = parseInt(e.target.value)}">
                        <option value="255" ?selected="${this.auxChannelPreset === 255}">Disabled</option>
                        ${[...Array(16).keys()].map((i) => html`
                            <option value="${i}" ?selected="${this.auxChannelPreset === i}">AUX${i + 1}</option>
                        `)}
                    </select>
                    <label for="auxPreset">Preset Control AUX Channel</label>
                </div>
                <p style="font-size: 0.9em; color: #666;">
                    Maps 0-25%=Preset0, 25-50%=Preset1, 50-75%=Preset2, 75-100%=Preset3
                </p>

                <div class="mui-select">
                    <select @change="${(e) => this.auxChannelBright = parseInt(e.target.value)}">
                        <option value="255" ?selected="${this.auxChannelBright === 255}">Disabled</option>
                        ${[...Array(16).keys()].map((i) => html`
                            <option value="${i}" ?selected="${this.auxChannelBright === i}">AUX${i + 1}</option>
                        `)}
                    </select>
                    <label for="auxBright">Brightness Control AUX Channel</label>
                </div>
                <p style="font-size: 0.9em; color: #666;">
                    Maps 0-100% = Brightness 0-100%
                </p>

                <h2>Global Brightness</h2>
                <div style="display: flex; align-items: center; gap: 10px;">
                    <input type="range" min="0" max="100"
                           .value="${this.currentBrightness}"
                           @change="${(e) => this.currentBrightness = parseInt(e.target.value)}"
                           style="flex: 1;"/>
                    <span style="min-width: 40px; text-align: right;">${this.currentBrightness}%</span>
                </div>

                <h2>Preset Editor</h2>
                <div class="mui-select">
                    <select @change="${(e) => { this.selectedPreset = parseInt(e.target.value); this.selectedStrip = 0; this.selectedPixel = 0; }}">
                        ${_renderOptions(['Preset 0', 'Preset 1', 'Preset 2', 'Preset 3'], this.selectedPreset)}
                    </select>
                    <label for="preset">Select Preset</label>
                </div>

                <h3>Strip Selection (Preset ${this.selectedPreset})</h3>
                <div class="mui-select">
                    <select @change="${(e) => { this.selectedStrip = parseInt(e.target.value); this.selectedPixel = 0; }}">
                        ${_renderOptions(['Strip 0', 'Strip 1', 'Strip 2', 'Strip 3'], this.selectedStrip)}
                    </select>
                    <label for="strip">Select Strip</label>
                </div>

                ${strip ? html`
                    <div class="mui-checkbox">
                        <input id="stripEnabled" type="checkbox" ?checked="${strip.enabled}"
                               @change="${(e) => this.toggleStripEnabled(e.target.checked)}"/>
                        <label for="stripEnabled">Enable this strip</label>
                    </div>
                ` : ''}

                <h3>Pixel Editor (Preset ${this.selectedPreset}, Strip ${this.selectedStrip})</h3>
                <div class="mui-select">
                    <select @change="${(e) => this.selectedPixel = parseInt(e.target.value)}">
                        ${[...Array(30).keys()].map((i) => html`<option value="${i}">Pixel ${i}</option>`)}
                    </select>
                    <label for="pixel">Select Pixel</label>
                </div>

                ${pixel ? this.renderPixelEditor(pixel) : ''}

                <button class="mui-btn mui-btn--primary" style="margin-top: 20px;"
                        @click="${this.save}">
                    Save LED Configuration
                </button>
            </div>
        `
    }

    renderPixelEditor(pixel) {
        if (!pixel) {
            return html``
        }
        return html`
            <div style="border: 1px solid #ddd; padding: 15px; margin: 15px 0; border-radius: 4px;">
                <h4>Color (RGB)</h4>
                <div style="display: flex; gap: 10px; align-items: center;">
                    <input type="color" .value="${this.rgbToHex(pixel.r, pixel.g, pixel.b)}"
                           @change="${(e) => this.updatePixelColor(e.target.value)}"
                           style="width: 60px; height: 40px; border: none; cursor: pointer;"/>
                    <span style="font-family: monospace;">${this.rgbToHex(pixel.r, pixel.g, pixel.b)}</span>
                </div>

                <h4>State</h4>
                <div class="mui-select">
                    <select @change="${(e) => this.updatePixelState(parseInt(e.target.value))}">
                        <option value="0" ?selected="${pixel.state === 0}">Off</option>
                        <option value="1" ?selected="${pixel.state === 1}">On (Always)</option>
                        <option value="2" ?selected="${pixel.state === 2}">Blinking</option>
                    </select>
                    <label for="state">Pixel State</label>
                </div>

                ${pixel.state === 2 ? html`
                    <h4>Blink Frequency</h4>
                    <div style="display: flex; align-items: center; gap: 10px;">
                        <input type="number" min="0.1" max="100" step="0.1"
                               .value="${(pixel.blinkFreqHz / 10).toFixed(1)}"
                               @change="${(e) => this.updateBlinkFreq(parseFloat(e.target.value))}"
                               style="width: 80px;"/>
                        <span>Hz (0.1-100)</span>
                    </div>
                ` : ''}

                <h4>Brightness</h4>
                <div style="display: flex; align-items: center; gap: 10px;">
                    <input type="range" min="0" max="255" .value="${pixel.brightness}"
                           @change="${(e) => this.updatePixelBrightness(parseInt(e.target.value))}"
                           style="flex: 1;"/>
                    <span style="min-width: 40px; text-align: right;">${pixel.brightness}</span>
                </div>
            </div>
        `
    }

    rgbToHex(r, g, b) {
        return '#' + [r, g, b].map(x => x.toString(16).padStart(2, '0')).join('')
    }

    hexToRgb(hex) {
        const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
        return result ? {
            r: parseInt(result[1], 16),
            g: parseInt(result[2], 16),
            b: parseInt(result[3], 16)
        } : {r: 0, g: 0, b: 0}
    }

    updatePixelColor(hexColor) {
        const rgb = this.hexToRgb(hexColor)
        const pixel = this.ledConfig.presets[this.selectedPreset].strips[this.selectedStrip].pixels[this.selectedPixel]
        pixel.r = rgb.r
        pixel.g = rgb.g
        pixel.b = rgb.b
        this.requestUpdate()
    }

    updatePixelState(state) {
        const pixel = this.ledConfig.presets[this.selectedPreset].strips[this.selectedStrip].pixels[this.selectedPixel]
        pixel.state = state
        this.requestUpdate()
    }

    updateBlinkFreq(freqHz) {
        const pixel = this.ledConfig.presets[this.selectedPreset].strips[this.selectedStrip].pixels[this.selectedPixel]
        pixel.blinkFreqHz = Math.round(freqHz * 10)  // Store as freq * 10
        this.requestUpdate()
    }

    updatePixelBrightness(brightness) {
        const pixel = this.ledConfig.presets[this.selectedPreset].strips[this.selectedStrip].pixels[this.selectedPixel]
        pixel.brightness = brightness
        this.requestUpdate()
    }

    toggleStripEnabled(enabled) {
        const strip = this.ledConfig.presets[this.selectedPreset].strips[this.selectedStrip]
        strip.enabled = enabled ? 1 : 0
        this.requestUpdate()
    }

    checkChanged() {
        // Check if any fields have changed from the loaded config
        if (!elrsState.config.ledConfig) return false
        const original = elrsState.config.ledConfig
        return (
            this.auxChannelPreset !== original.auxChannelPreset ||
            this.auxChannelBright !== original.auxChannelBright ||
            this.currentBrightness !== original.currentBrightness ||
            JSON.stringify(this.ledConfig) !== JSON.stringify(original)
        )
    }

    save() {
        // Update aux channels in config
        this.ledConfig.auxChannelPreset = this.auxChannelPreset
        this.ledConfig.auxChannelBright = this.auxChannelBright
        this.ledConfig.currentBrightness = this.currentBrightness

        // Save via POST to /api/led/config
        postJSON('/api/led/config', this.ledConfig, {
            onload: async () => {
                await infoAlert('LED Configuration', 'LED configuration saved successfully')
                location.reload()
            },
            onerror: async (xhr) => {
                await errorAlert('Error', 'Failed to save LED configuration: ' + (xhr.responseText || 'Unknown error'))
            }
        })
    }
}
