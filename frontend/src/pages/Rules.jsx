import RuleEditor from "../components/RuleEditor.jsx";
import RuleList from "../components/RuleList.jsx";

export default function Rules({ source, setSource, result, loading, compile }) {
  return (
    <div className="cols2">
      <RuleEditor
        source={source}
        setSource={setSource}
        onCompile={compile}
        loading={loading}
        diagnostics={result?.diagnostics}
      />
      <RuleList rules={result?.rules} />
    </div>
  );
}