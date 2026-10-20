import CompilationView from "../components/CompilationView.jsx";

export default function Compiler({ result, loading, compile }) {
  return (
    <div className="stack">
      <div className="toolbar">
        <button className="btn" onClick={compile} disabled={loading}>
          {loading ? "Compiling..." : "Recompile"}
        </button>
      </div>
      <CompilationView result={result} />
    </div>
  );
}