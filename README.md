# minimale

Experimental project. The goal is to compile a React-inspired description of
components:

```js
export default component HelloWorld {
  name: string;

  render() {
    return (
      <p>Hello, {this.name}!</p>
    );
  }

  setName(name: string) {
    this.name = name;
  }
}
```

Into a JavaScript representation that does the minimal work possible, without
a runtime such as React. The API would then look like that:

```js
const HelloWorld = require('./HelloWorld');

const hw = new HelloWorld(document.getElementById('root'), {name: 'world'});

// That changes the node containing the name directly.
hw.setName('beep');
```

## Development

    yarn
    ./configure.js
    upd dist/minimale && dist/minimale examples/HelloWorld.mn

