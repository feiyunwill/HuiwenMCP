"""HNP 固定的 PDF 命令入口；业务接口由 huiwen_converter.pdf_to_md 提供。"""

import sys
import logging


def main() -> int:
    if len(sys.argv) != 3:
        raise ValueError("需要输入 PDF 路径和输出 MD 路径")
    logging.basicConfig(
        level=logging.INFO,
        format="[HUIWEN_PY][%(levelname)s] %(name)s %(message)s",
        stream=sys.stderr,
    )
    import markitdown
    from markitdown.converters import _pdf_converter
    from huiwen_converter import pdf_to_md

    logger = logging.getLogger("huiwen.convert")
    logger.info(
        "RUNTIME python=%s markitdown=%s module=%s pdf_module=%s algorithm=%s",
        sys.version.split()[0],
        getattr(markitdown, "__version__", "unknown"),
        markitdown.__file__,
        _pdf_converter.__file__,
        getattr(_pdf_converter, "PDF_STRUCTURE_ALGORITHM", "upstream-no-structure"),
    )
    pdf_to_md(sys.argv[1], sys.argv[2])
    logger.info("CONVERT_DONE")
    print("HUIWEN_CONVERT_OK", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        logging.getLogger("huiwen.convert").exception("CONVERT_FAILED")
        print(f"PDF 转换失败：{error}", file=sys.stderr, flush=True)
        raise SystemExit(1)
