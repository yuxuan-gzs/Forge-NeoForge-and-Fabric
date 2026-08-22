package antlr;

import org.antlr.v4.runtime.*;

public abstract class JavaParserBase extends Parser {
    public JavaParserBase(TokenStream input) {
        super(input);
    }

    /**
     * 检查当前 token 是否不是标识符赋值
     * 在 annotationFieldValue 规则中使用
     */
    public boolean IsNotIdentifierAssign() {
        // 实现逻辑：检查当前 token 后面是否跟着 '='
        // 如果是标识符赋值，返回 false；否则返回 true
        Token token = getCurrentToken();
        if (token == null) {
            return true;
        }
        
        // 获取下一个 token
        int nextTokenIndex = token.getTokenIndex() + 1;
        if (nextTokenIndex < getInputStream().size()) {
            Token nextToken = getInputStream().get(nextTokenIndex);
            // 如果下一个 token 是 '='，则这是标识符赋值
            if (nextToken.getText().equals("=")) {
                return false;
            }
        }
        return true;
    }

    /**
     * 检查是否是最后一个 record 组件
     * 在 recordComponentList 规则中使用
     */
    public boolean DoLastRecordComponent() {
        // 实现逻辑：检查当前是否在 record 组件列表的末尾
        // 这通常涉及到检查下一个 token 是否是 ')' 来结束 record header
        Token token = getCurrentToken();
        if (token == null) {
            return false;
        }
        
        // 检查下一个有意义的 token
        int nextTokenIndex = token.getTokenIndex() + 1;
        while (nextTokenIndex < getInputStream().size()) {
            Token nextToken = getInputStream().get(nextTokenIndex);
            // 跳过空白字符
            if (nextToken.getChannel() == Token.DEFAULT_CHANNEL) {
                // 如果下一个 token 是 ')'，说明这是最后一个组件
                return nextToken.getText().equals(")");
            }
            nextTokenIndex++;
        }
        return false;
    }
}