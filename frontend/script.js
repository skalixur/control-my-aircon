const usernameInput = document.querySelector("#username");
const passwordInput = document.querySelector("#password");
const locationInput = document.querySelector("#location");
const serverInput = document.querySelector("#server");
const connectButton = document.querySelector(".connect");
const connectionStatus = document.querySelector(".connection-status");

const clientOptions = {
  username: "",
  password: "",
};

const options = {
  mqttAddr: "",
  location: "",
};

usernameInput.addEventListener("change", () => {
  clientOptions.username = usernameInput.value;
});

passwordInput.addEventListener("change", () => {
  clientOptions.password = passwordInput.value;
});

locationInput.addEventListener("change", () => {
  options.location = locationInput.value;
});

connectButton.addEventListener("click", () => {
  go();
});

let isAcAvailable = false;
const stateTopic = `${options.location}/ac/state`;
const commandTopic = `${options.location}/ac/set`;

function bad() {
  connectionStatus.innerText = "disconnected";
  connectionStatus.classList.add("bad");
  connectionStatus.classList.remove("good");
}

/*
example
{
    "currentTemperature": -100,
    "temperature": 18,
    "fanMode": "Auto",
    "mode": "Cool",
    "swingHorizontalMode": "Off",
    "swingMode": "Auto",
    "protocol": "PANASONIC_AC",
    "model": 6,
    "quiet": false,
    "turbo": true,
    "econo": false,
    "light": false,
    "filter": false,
    "clean": false,
    "beep": false,
    "sleep": -1,
    "clock": -1,
    "command": "Control",
    "iFeel": false,
    "echo": true,
    "ignoreWindow": 300
}
*/

const temperatureState = document.querySelector(".temperature-state");
const fanModeState = document.querySelector(".fan-mode-state");
const modeState = document.querySelector(".mode-state");
const swingHorizontalModeState = document.querySelector(
  ".swing-horizontal-state",
);
const swingVerticalModeState = document.querySelector(".swing-vertical-state");
const quietState = document.querySelector(".quiet-state");
const turboState = document.querySelector(".turbo-state");
const econoState = document.querySelector(".econo-state");
const lightState = document.querySelector(".light-state");
const filterState = document.querySelector(".filter-state");
const cleanState = document.querySelector(".clean-state");
const beepState = document.querySelector(".beep-state");

function boolToOnOffString(bool) {
  return bool ? "On" : "Off";
}

function handleState(state) {
  modeState.innerText = state.mode;
  temperatureState.innerText = state.temperature;
  fanModeState.innerText = state.fanMode;
  swingHorizontalModeState.innerText = state.swingHorizontalMode;
  swingVerticalModeState.innerText = state.swingMode;

  quietState.innerText = boolToOnOffString(state.quiet);
  turboState.innerText = boolToOnOffString(state.turbo);
  econoState.innerText = boolToOnOffString(state.econo);
  lightState.innerText = boolToOnOffString(state.light);
  filterState.innerText = boolToOnOffString(state.filter);
  cleanState.innerText = boolToOnOffString(state.clean);
  beepState.innerText = boolToOnOffString(state.beep);
  console.log(state);
}

const modeInput = document.querySelector("#mode");
const fanModeInput = document.querySelector("#fan_mode");
const temperatureInput = document.querySelector("#temperature");
const swingHorizontalInput = document.querySelector("#swing_horizontal");
const swingVerticalInput = document.querySelector("#swing_vertical");
const quietInput = document.querySelector("#quiet");
const turboInput = document.querySelector("#turbo");
const econoInput = document.querySelector("#econo");
const lightInput = document.querySelector("#light");
const filterInput = document.querySelector("#filter");
const cleanInput = document.querySelector("#clean");
const beepInput = document.querySelector("#beep");

let currentClient = null;

function publish(topic, value) {
  if (currentClient && currentClient.connected) {
    currentClient.publish(topic, value);
  }
}

function boolToOnOff(val) {
  return val ? "ON" : "OFF";
}

function setupControlHandlers(client) {
  currentClient = client;

  if (modeInput) {
    modeInput.addEventListener("change", () => {
      publish(`${commandTopic}/mode`, modeInput.value);
    });
  }
  if (fanModeInput) {
    fanModeInput.addEventListener("change", () => {
      publish(`${commandTopic}/fan_mode`, fanModeInput.value);
    });
  }
  if (temperatureInput) {
    temperatureInput.addEventListener("change", () => {
      publish(`${commandTopic}/temperature`, temperatureInput.value);
    });
  }
  if (swingHorizontalInput) {
    swingHorizontalInput.addEventListener("change", () => {
      publish(
        `${commandTopic}/swing_horizontal_mode`,
        swingHorizontalInput.value,
      );
    });
  }
  if (swingVerticalInput) {
    swingVerticalInput.addEventListener("change", () => {
      publish(`${commandTopic}/swing_mode`, swingVerticalInput.value);
    });
  }
  if (quietInput) {
    quietInput.addEventListener("change", () => {
      publish(`${commandTopic}/quiet`, boolToOnOff(quietInput.value === "on"));
    });
  }
  if (turboInput) {
    console.log("turbo changed");
    turboInput.addEventListener("change", () => {
      publish(`${commandTopic}/turbo`, boolToOnOff(turboInput.value === "on"));
    });
  }
  if (econoInput) {
    econoInput.addEventListener("change", () => {
      publish(`${commandTopic}/econo`, boolToOnOff(econoInput.value === "on"));
    });
  }
  if (lightInput) {
    lightInput.addEventListener("change", () => {
      publish(`${commandTopic}/light`, boolToOnOff(lightInput.value === "on"));
    });
  }
  if (filterInput) {
    filterInput.addEventListener("change", () => {
      publish(
        `${commandTopic}/filter`,
        boolToOnOff(filterInput.value === "on"),
      );
    });
  }
  if (cleanInput) {
    cleanInput.addEventListener("change", () => {
      publish(`${commandTopic}/clean`, boolToOnOff(cleanInput.value === "on"));
    });
  }
  if (beepInput) {
    beepInput.addEventListener("change", () => {
      publish(`${commandTopic}/beep`, boolToOnOff(beepInput.value === "on"));
    });
  }
}

function go() {
  options.location = locationInput.value;
  options.mqttAddr = serverInput.value;
  const client = mqtt.connect(options.mqttAddr, clientOptions);
  bad();
  connectionStatus.innerText = "failed";

  client.on("connect", () => {
    connectionStatus.innerText = "connected";
    connectionStatus.classList.remove("bad");
    connectionStatus.classList.add("good");
    console.log("connected");
    client.subscribe(`${stateTopic}/#`, (err) => {
      if (err) console.log(err);
    });
    setupControlHandlers(client);
  });

  client.on("disconnect", () => {
    bad();
  });

  client.on("offline", () => {
    bad();
  });

  client.on("error", () => {
    bad();
  });

  client.on("message", (topic, message) => {
    const cleanedTopic = topic.substring(stateTopic.length);
    message = message.toString();

    if (topic === stateTopic) {
      handleState(JSON.parse(message));
      return;
    }

    if (cleanedTopic === "/availability") {
      isAcAvailable = message === "online";
      return;
    }

    console.log(message);
  });
}
