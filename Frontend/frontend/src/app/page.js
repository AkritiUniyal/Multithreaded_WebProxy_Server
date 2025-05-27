

export default function Home() {
  return (
    <main className="flex min-h-screen flex-col items-center justify-between p-24">
      <h1 className="text-4xl font-bold">Welcome to the Multithreaded Web Proxy Server</h1>
      <p className="mt-4 text-lg">This is a simple web proxy server that can handle multiple requests concurrently.</p>
      <p className="mt-2 text-lg">You can use it to test your web applications or to browse the web anonymously.</p>
      <p className="mt-2 text-lg">To get started, please enter a URL in the address bar of your browser.</p>
      <p className="mt-2 text-lg">The server will forward your request and return the response.</p>
      <p className="mt-2 text-lg">Enjoy your browsing experience!</p>
      

    </main>

  );
}
