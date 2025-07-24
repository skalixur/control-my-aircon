const usernameInput = document.querySelector("#username");
const passwordInput = document.querySelector("#password");
const locationInput = document.querySelector("#location");
const connectButton = document.querySelector("#connect");
const connectionStatus = document.querySelector(".connection-status");

const clientOptions = {
  username: "",
  password: "",
};

const options = {
  mqttAddr: "ws://localhost:9001",
  location: "bedroom",
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

function go() {
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
    // console.log(`New message on topic: ${cleanedTopic}`);

    message = message.toString();

    if (topic === stateTopic) {
      // Handle full JSON state
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

function bad() {
  connectionStatus.innerText = "disconnected";
  connectionStatus.classList.add("bad");
  connectionStatus.classList.remove("good");
}

function handleState(state) {
  //
  console.log(state);
}
