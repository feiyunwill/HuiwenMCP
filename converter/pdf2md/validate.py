"""Convert a text PDF and independently check content; optional QA dependencies only.

Run with converter/ on PYTHONPATH and the checkout's MarkItDown installed editable.
This never modifies the input PDF or an existing reference Markdown file.
"""

import argparse
from collections import Counter
import hashlib
import json
import logging
from pathlib import Path
import re

import pdfplumber
from pypdf import PdfReader

from huiwen_converter import pdf_to_md
from markitdown.converters._pdf_converter import PDF_STRUCTURE_ALGORITHM


def content_counts(text):
    return Counter(char for char in text if not char.isspace() and char not in "•●▪◦")


def markdown_content(text):
    """Remove only generated markup; preserve literal markup inside code fences."""
    output, fence = [], ""
    for line in text.splitlines():
        if fence:
            if line == fence:
                fence = ""
            else:
                output.append(line)
            continue
        match = re.match(r"^(`{3,})\w*$", line)
        if match:
            fence = match[1]
            continue
        if re.fullmatch(r"<!-- PDF page \d+ -->", line):
            continue
        if re.fullmatch(r"\|[ |:\-]+\|", line):
            continue
        if line.startswith("|") and line.endswith("|"):
            # Split on structural pipes only, keeping escaped content pipes.
            cells = re.split(r"(?<!\\)\|", line[1:-1])
            line = "".join(cells).replace(r"\|", "|").replace("\\\\", "\\")
        line = re.sub(r"^#{1,6}\s+", "", line)
        line = re.sub(r"^-\s+", "", line)
        line = line.replace("<small>", "").replace("</small>", "").replace("<br>", "")
        output.append(line.replace("**", ""))
    return "\n".join(output)


def compare(expected, actual):
    return dict(
        equal=expected == actual,
        expected_characters=sum(expected.values()),
        actual_characters=sum(actual.values()),
        missing=dict(expected - actual),
        extra=dict(actual - expected),
    )


def render_overview(source, output):
    import pypdfium2 as pdfium
    from PIL import Image, ImageDraw

    thumbnails = []
    with pdfium.PdfDocument(source) as pdf:
        for index in range(len(pdf)):
            page = pdf[index]
            bitmap = page.render(scale=1.3)
            image = bitmap.to_pil().copy()
            bitmap.close()
            page.close()
            image.save(output / f"原文-第{index + 1}页.png")
            image.thumbnail((397, 514))
            tile = Image.new("RGB", (417, 544), "#dddddd")
            tile.paste(image, (10, 25))
            ImageDraw.Draw(tile).text((10, 5), f"Page {index + 1}", fill="black")
            thumbnails.append(tile)
    overview = Image.new("RGB", (417 * 3, 544 * ((len(thumbnails) + 2) // 3)), "white")
    for index, tile in enumerate(thumbnails):
        overview.paste(tile, ((index % 3) * 417, (index // 3) * 544))
    overview.save(output / "原文总览.png")


def validate(source, output, render=False):
    source, output = source.resolve(), output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    target = output / "新算法.md"
    pdf_to_md(source, target)
    markdown = target.read_text(encoding="utf-8")
    extracted_pages = re.split(r"<!-- PDF page \d+ -->", markdown)[1:]
    reader = PdfReader(source)
    reports = []
    with pdfplumber.open(source) as pdf:
        if len(extracted_pages) != len(pdf.pages):
            raise ValueError("Markdown page boundaries do not match the PDF")
        for index, (page, page_text) in enumerate(zip(pdf.pages, extracted_pages)):
            expected = content_counts("".join(char["text"] for char in page.chars))
            reports.append(
                dict(
                    page=index + 1,
                    markdown=compare(
                        expected, content_counts(markdown_content(page_text))
                    ),
                    pypdf=compare(
                        expected,
                        content_counts(reader.pages[index].extract_text() or ""),
                    ),
                    ruled_tables=len(
                        [
                            table
                            for table in page.find_tables()
                            if len(table.rows) >= 2
                            and max(len(row.cells) for row in table.rows) >= 2
                        ]
                    ),
                )
            )
            page.close()
    json_reports = []
    for code in re.findall(r"```json\n(.*?)\n```", markdown, re.S):
        try:
            json.loads(code)
            json_reports.append({"valid": True})
        except ValueError as error:
            json_reports.append({"valid": False, "error": str(error)})
    after = hashlib.sha256(source.read_bytes()).hexdigest()
    report = dict(
        algorithm=PDF_STRUCTURE_ALGORITHM,
        source=str(source),
        output=str(target),
        source_sha256_before=before,
        source_sha256_after=after,
        source_unchanged=before == after,
        nonblank_characters=sum(
            page["markdown"]["expected_characters"] for page in reports
        ),
        pages=reports,
        json_blocks=json_reports,
        content_counts_equal=all(
            page["markdown"]["equal"] and page["pypdf"]["equal"] for page in reports
        ),
        limitation="Character multisets do not prove reading order; visual inspection is still required. No OCR is used.",
    )
    if render:
        render_overview(str(source), output)
    (output / "校验报告.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(
        json.dumps(
            {
                "output": str(target),
                "content_counts_equal": report["content_counts_equal"],
                "source_unchanged": report["source_unchanged"],
            },
            ensure_ascii=False,
        )
    )
    return (
        0
        if report["content_counts_equal"]
        and report["source_unchanged"]
        and all(item["valid"] for item in json_reports)
        else 1
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pdf", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--render", action="store_true")
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
    raise SystemExit(validate(args.pdf, args.output_dir, args.render))
